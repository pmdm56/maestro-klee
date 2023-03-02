#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"

#include "../../visitor.h"
#include "../target_code_generator.h"

#include "constants.h"
#include "pipeline/pipeline.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

class TofinoGenerator : public Target {
private:
  HeadersDefinitions headers_definitions;
  IngressParser ingress_parser;
  Ingress ingress;
  IngressHeaders ingress_headers;

public:
  TofinoGenerator()
      : Target(GET_BOILERPLATE_PATH(TOFINO_BOILERPLATE_FILE)),
      headers_definitions(
            code_builder.get_indentation_level(MARKER_HEADERS_DEFINITIONS)),
        ingress_parser(
            code_builder.get_indentation_level(MARKER_INGRESS_PARSE_HEADERS)),
        ingress(code_builder.get_indentation_level(MARKER_INGRESS_APPLY)),
        ingress_headers(
            code_builder.get_indentation_level(MARKER_INGRESS_HEADERS)) {}

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;

  void visit(const targets::tofino::If *node) override;
  void visit(const targets::tofino::Then *node) override;
  void visit(const targets::tofino::Else *node) override;
  void visit(const targets::tofino::Forward *node) override;
  void visit(const targets::tofino::EthernetConsume *node) override;

  std::string transpile(klee::ref<klee::Expr> expr) const;
  const tofino::variable_t *search_variable(std::string symbol) const;
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse
