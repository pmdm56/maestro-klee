#include "util.h"
#include "../../../symbex.h"
#include "../../execution_plan_node.h"
#include "../../modules/module.h"

namespace synapse {
namespace synthesizer {

bool pending_packet_borrow_next_chunk(const ExecutionPlanNode *ep_node,
                                      synapse::TargetType target) {
  assert(ep_node);
  // std::vector<ExecutionPlanNode_ptr> nodes;
  std::vector<const ExecutionPlanNode *> nodes{ep_node};

  auto next = ep_node->get_next();

  for (auto n : next) {
    nodes.push_back(n.get());
  }

  while (nodes.size()) {
    auto node = nodes[0];
    nodes.erase(nodes.begin());

    auto module = node->get_module();
    assert(module);

    if (module->get_target() != target) {
      continue;
    }

    auto bdd_node = module->get_node();
    assert(bdd_node);

    if (bdd_node->get_type() == BDD::Node::NodeType::CALL) {
      auto call_node = static_cast<const BDD::Call *>(bdd_node.get());
      if (call_node->get_call().function_name == symbex::FN_BORROW_CHUNK) {
        return true;
      }
    }

    auto branches = node->get_next();

    for (auto n : branches) {
      nodes.push_back(n.get());
    }
  }

  return false;
}

std::string get_label(BDD::symbols_t symbols, std::string base) {
  auto found_it = std::find_if(
      symbols.begin(), symbols.end(),
      [&](const BDD::symbol_t &symbol) { return symbol.label_base == base; });

  assert(found_it != symbols.end());
  return found_it->label;
}

BDD::symbol_t get_symbol(BDD::symbols_t symbols, std::string base) {
  auto found_it = std::find_if(
      symbols.begin(), symbols.end(),
      [&](const BDD::symbol_t &symbol) { return symbol.label_base == base; });

  assert(found_it != symbols.end());
  return *found_it;
}

} // namespace synthesizer
} // namespace synapse