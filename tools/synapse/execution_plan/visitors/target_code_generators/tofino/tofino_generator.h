#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"

#include "../../visitor.h"
#include "../target_code_generator.h"

#include "constants.h"
#include "pipeline/pipeline.h"

#include "transpiler.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

class TofinoGenerator : public Target {
private:
  Ingress ingress;
  Transpiler transpiler;

public:
  TofinoGenerator()
      : Target(GET_BOILERPLATE_PATH(TOFINO_BOILERPLATE_FILE)),
        ingress(
            code_builder.get_indentation_level(MARKER_INGRESS_STATE),
            code_builder.get_indentation_level(MARKER_INGRESS_APPLY),
            code_builder.get_indentation_level(MARKER_INGRESS_METADATA),
            code_builder.get_indentation_level(MARKER_HEADERS_DEFINITIONS),
            code_builder.get_indentation_level(MARKER_INGRESS_HEADERS),
            code_builder.get_indentation_level(MARKER_INGRESS_PARSE_HEADERS)),
        transpiler(*this) {}

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;

  void visit(const targets::tofino::If *node) override;
  void visit(const targets::tofino::Then *node) override;
  void visit(const targets::tofino::Else *node) override;
  void visit(const targets::tofino::Forward *node) override;
  void visit(const targets::tofino::EthernetConsume *node) override;
  void visit(const targets::tofino::EthernetModify *node) override;
  void visit(const targets::tofino::TableLookup *node) override;
  void visit(const targets::tofino::Drop *node) override;

  std::string transpile(klee::ref<klee::Expr> expr);

  variable_query_t search_variable(std::string symbol) const;
  variable_query_t search_variable(klee::ref<klee::Expr> expr) const;

  Variable allocate_key_byte(int byte);
  std::vector<Variable> allocate_key_bytes(klee::ref<klee::Expr> expr);
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse
