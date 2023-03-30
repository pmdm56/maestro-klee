#pragma once

#include "../../execution_plan.h"
#include "code_builder.h"

namespace synapse {
namespace synthesizer {

class PendingIfs {
private:
  std::stack<bool> pending;
  CodeBuilder &builder;

public:
  PendingIfs(CodeBuilder &_builder) : builder(_builder) {}

  void push() { pending.push(true); }
  void pop() { pending.pop(); }

  // returns number of closed stacked blocks
  int close() {
    int closed = 0;

    if (!pending.size()) {
      return closed;
    }

    while (pending.size()) {
      builder.dec_indentation();
      builder.indent();
      builder.append("}");
      builder.append_new_line();

      closed++;

      auto if_clause = pending.top();
      pending.pop();

      if (if_clause) {
        pending.push(false);
        break;
      }
    }

    return closed;
  }
};

bool pending_packet_borrow_next_chunk(const ExecutionPlanNode *ep_node,
                                      synapse::TargetType target);

bool has_label(BDD::symbols_t symbols, std::string base);
std::string get_label(BDD::symbols_t symbols, std::string base);
BDD::symbol_t get_symbol(BDD::symbols_t symbols, std::string base);
bool is_primitive_type(bits_t size);

} // namespace synthesizer
} // namespace synapse