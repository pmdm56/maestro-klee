#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"
#include "../synthesizer.h"
#include "constants.h"
#include "pipeline/pipeline.h"
#include "transpiler.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

class TofinoGenerator : public Synthesizer {
  friend Transpiler;

private:
  Ingress ingress;
  Transpiler transpiler;

public:
  TofinoGenerator()
      : Synthesizer(GET_BOILERPLATE_PATH(TOFINO_BOILERPLATE_FILE)),
        ingress(get_indentation_level(MARKER_INGRESS_STATE),
                get_indentation_level(MARKER_INGRESS_APPLY),
                get_indentation_level(MARKER_INGRESS_METADATA)),
        transpiler(*this) {}

  virtual void generate(ExecutionPlan &target_ep) override { visit(target_ep); }

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;

  void visit(const targets::tofino::If *node) override;
  void visit(const targets::tofino::IfHeaderValid *node) override;
  void visit(const targets::tofino::Then *node) override;
  void visit(const targets::tofino::Else *node) override;
  void visit(const targets::tofino::Forward *node) override;
  void visit(const targets::tofino::EthernetConsume *node) override;
  void visit(const targets::tofino::EthernetModify *node) override;
  void visit(const targets::tofino::IPv4Consume *node) override;
  void visit(const targets::tofino::IPv4Modify *node) override;
  void visit(const targets::tofino::IPv4OptionsConsume *node) override;
  void visit(const targets::tofino::TCPUDPConsume *node) override;
  void visit(const targets::tofino::TCPUDPModify *node) override;
  void visit(const targets::tofino::IPv4TCPUDPChecksumsUpdate *node) override;
  void visit(const targets::tofino::TableLookup *node) override;
  void visit(const targets::tofino::Drop *node) override;
  void visit(const targets::tofino::Ignore *node) override;
  void visit(const targets::tofino::SendToController *node) override;

  std::string transpile(klee::ref<klee::Expr> expr);

  variable_query_t search_variable(std::string symbol) const;
  variable_query_t search_variable(klee::ref<klee::Expr> expr) const;

private:
  void
  visit_if_multiple_conditions(std::vector<klee::ref<klee::Expr>> conditions);
  void visit_if_simple_condition(klee::ref<klee::Expr> condition);
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse
