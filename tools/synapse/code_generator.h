#pragma once

#include "execution_plan/execution_plan.h"
#include "modules/modules.h"

#include "execution_plan/visitors/target_code_generators/target_code_generators.h"

#include <sys/stat.h>
#include <vector>

namespace synapse {

class CodeGenerator {
private:
  typedef ExecutionPlan (CodeGenerator::*ExecutionPlanTargetExtractor)(
      const ExecutionPlan &) const;
  typedef std::shared_ptr<synapse::synthesizer::Target> TargetCodeGenerator_ptr;

  struct target_helper_t {
    ExecutionPlanTargetExtractor extractor;
    TargetCodeGenerator_ptr generator;

    target_helper_t(ExecutionPlanTargetExtractor _extractor)
        : extractor(_extractor) {}

    target_helper_t(ExecutionPlanTargetExtractor _extractor,
                    TargetCodeGenerator_ptr _generator)
        : extractor(_extractor), generator(_generator) {}
  };

  std::vector<target_helper_t> target_helpers_loaded;
  std::map<Target, target_helper_t> target_helpers_bank;

private:
  ExecutionPlan x86_bmv2_extractor(const ExecutionPlan &execution_plan) const;
  ExecutionPlan bmv2_extractor(const ExecutionPlan &execution_plan) const;
  ExecutionPlan fpga_extractor(const ExecutionPlan &execution_plan) const;
  ExecutionPlan tofino_extractor(const ExecutionPlan &execution_plan) const;
  ExecutionPlan netronome_extractor(const ExecutionPlan &execution_plan) const;

  std::string directory;

public:
  CodeGenerator(const std::string &_directory) : directory(_directory) {
    target_helpers_bank = {
        {Target::x86_BMv2,
         target_helper_t(
             &CodeGenerator::x86_bmv2_extractor,
             std::make_shared<synapse::synthesizer::x86BMv2Generator>())},

        {Target::BMv2,
         target_helper_t(
             &CodeGenerator::bmv2_extractor,
             std::make_shared<synapse::synthesizer::BMv2Generator>())},

        {Target::FPGA, target_helper_t(&CodeGenerator::fpga_extractor)},

        {Target::Tofino,
         target_helper_t(
             &CodeGenerator::tofino_extractor,
             std::make_shared<synapse::synthesizer::tofino::TofinoGenerator>())},

        {Target::Netronome,
         target_helper_t(&CodeGenerator::netronome_extractor)},
    };
  }

  void add_target(Target target) {
    auto found_it = target_helpers_bank.find(target);
    assert(found_it != target_helpers_bank.end() &&
           "Target not found in target_extractors_bank of CodeGenerator");
    assert(found_it->second.generator);

    if (!directory.size()) {
      target_helpers_loaded.push_back(found_it->second);
      return;
    }

    auto output_file = directory + "/";

    switch (target) {
    case Target::x86_BMv2:
      output_file += "x86-bmv2.c";
      break;
    case Target::BMv2:
      output_file += "bmv2.p4";
      break;
    case Target::FPGA:
      output_file += "fpga.v";
      break;
    case Target::Tofino:
      output_file += "tofino.p4";
      break;
    case Target::Netronome:
      output_file += "netronome.c";
      break;
    }

    found_it->second.generator->output_to_file(output_file);
    target_helpers_loaded.push_back(found_it->second);
  }

  void generate(const ExecutionPlan &execution_plan) {
    for (auto helper : target_helpers_loaded) {
      auto &extractor = helper.extractor;
      auto &generator = helper.generator;

      auto extracted_ep = (this->*extractor)(execution_plan);
      generator->generate(extracted_ep, execution_plan);
    }
  }
};

} // namespace synapse