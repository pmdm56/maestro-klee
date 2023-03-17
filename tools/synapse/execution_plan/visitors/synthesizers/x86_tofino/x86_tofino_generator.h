#pragma once

#include <sstream>
#include <vector>

#include "../../../../log.h"
#include "../../../execution_plan.h"
#include "../code_builder.h"
#include "../synthesizer.h"
#include "../util.h"
#include "constants.h"
#include "transpiler.h"

#include "domain/headers.h"
#include "domain/stack.h"
#include "domain/variable.h"

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

class x86TofinoGenerator : public Synthesizer {
  friend Transpiler;

private:
  CodeBuilder state_decl_builder;
  CodeBuilder state_init_builder;
  CodeBuilder nf_proces_builder;

  Transpiler transpiler;

  Headers headers;
  stack_t local_vars;
  PendingIfs pending_ifs;

public:
  x86TofinoGenerator()
      : Synthesizer(GET_BOILERPLATE_PATH(BOILERPLATE_FILE)),
        state_decl_builder(get_indentation_level(MARKER_STATE_DECL)),
        state_init_builder(get_indentation_level(MARKER_STATE_INIT)),
        nf_proces_builder(get_indentation_level(MARKER_NF_PROCESS)),
        transpiler(*this), pending_ifs(nf_proces_builder) {

    std::stringstream in_port_label_builder;
    in_port_label_builder << HDR_CPU_VARIABLE;
    in_port_label_builder << "->";
    in_port_label_builder << HDR_CPU_IN_PORT_FIELD;

    auto in_port_var = Variable(in_port_label_builder.str(), 16,
                                {symbex::PORT, symbex::PORT2});

    local_vars.append(in_port_var);
  }

  std::string transpile(klee::ref<klee::Expr> expr);
  virtual void generate(ExecutionPlan &target_ep) override { visit(target_ep); }

  variable_query_t search_variable(std::string symbol) const;
  variable_query_t search_variable(klee::ref<klee::Expr> expr) const;

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;

  void visit(const targets::x86_tofino::CurrentTime *node) override;
  void visit(const targets::x86_tofino::PacketParseCPU *node) override;

  void visit(const targets::x86_tofino::Drop *node) override;
  void visit(const targets::x86_tofino::ForwardThroughTofino *node) override;
  void visit(const targets::x86_tofino::PacketParseEthernet *node) override;
  void visit(const targets::x86_tofino::PacketModifyEthernet *node) override;
  void visit(const targets::x86_tofino::If *node) override;
  void visit(const targets::x86_tofino::Then *node) override;
  void visit(const targets::x86_tofino::Else *node) override;
};

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse