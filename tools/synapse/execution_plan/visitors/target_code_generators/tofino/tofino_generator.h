#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"

#include "../../visitor.h"
#include "../target_code_generator.h"

#include "pipeline/pipeline.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

constexpr char TOFINO_BOILERPLATE_FILE[] = "boilerplate.p4";

constexpr char MARKER_INGRESS_METADATA[] = "INGRESS METADATA";
constexpr char MARKER_INGRESS_HEADERS[] = "INGRESS HEADERS";
constexpr char MARKER_EGRESS_METADATA[] = "EGRESS METADATA";
constexpr char MARKER_EGRESS_HEADERS[] = "EGRESS HEADERS";
constexpr char MARKER_INGRESS_PARSE_HEADERS[] = "INGRESS PARSE HEADERS";
constexpr char MARKER_INGRESS_STATE[] = "INGRESS STATE";
constexpr char MARKER_INGRESS_APPLY[] = "INGRESS APPLY";

class TofinoGenerator : public Target {
private:
  tofino::IngressParser ingress_parser;
  tofino::Ingress ingress;
  bool parsing_headers;

public:
  TofinoGenerator()
      : Target(GET_BOILERPLATE_PATH(TOFINO_BOILERPLATE_FILE)),
        ingress_parser(
            code_builder.get_indentation_level(MARKER_INGRESS_PARSE_HEADERS)),
        ingress(code_builder.get_indentation_level(MARKER_INGRESS_APPLY)),
        parsing_headers(true) {}

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
