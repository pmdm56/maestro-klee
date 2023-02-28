#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"

#include "../../visitor.h"
#include "../target_code_generator.h"

#include "pipeline/pipeline.h"
#include "transpiler.h"

#define TOFINO_BOILERPLATE_FILE "boilerplate.p4"

namespace synapse {
namespace synthesizer {

class TofinoGenerator : public Target {
private:
  tofino::IngressParser ingress_parser;
  tofino::Ingress ingress;
  tofino::Transpiler transpiler;

public:
  TofinoGenerator() : Target(GET_BOILERPLATE_PATH(TOFINO_BOILERPLATE_FILE)) {}

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;
  
  void visit(const targets::tofino::If *node) override;
  void visit(const targets::tofino::Then *node) override;
  void visit(const targets::tofino::Else *node) override;
  void visit(const targets::tofino::Forward *node) override;
  void visit(const targets::tofino::EthernetConsume *node) override;
};

} // namespace synthesizer
} // namespace synapse
