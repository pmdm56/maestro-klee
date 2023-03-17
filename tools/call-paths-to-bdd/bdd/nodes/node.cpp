#include "node.h"
#include "branch.h"
#include "call.h"

#include "../symbol-factory.h"

namespace BDD {

symbols_t Node::get_generated_symbols() const {
  symbols_t symbols;
  const Node *node = this;

  // hack: symbols always known
  klee::ref<klee::Expr> empty_expr;
  symbols.emplace("VIGOR_DEVICE", "VIGOR_DEVICE", empty_expr);
  symbols.emplace("pkt_len", "pkt_len", empty_expr);
  symbols.emplace("data_len", "data_len", empty_expr);
  symbols.emplace("received_a_packet", "received_a_packet", empty_expr);

  while (node) {
    if (node->get_type() == Node::NodeType::CALL) {
      const Call *call = static_cast<const Call *>(node);
      auto more_symbols = call->get_node_generated_symbols();

      for (auto symbol : more_symbols) {
        symbols.insert(symbol);
      }
    }

    node = node->get_prev().get();
  }

  return symbols;
}

symbols_t Node::get_node_generated_symbols() const { return symbols_t(); }

void Node::update_id(uint64_t new_id) {
  SymbolFactory factory;
  auto symbols = factory.get_symbols(this);

  id = new_id;

  if (symbols.size() == 0) {
    return;
  }

  kutil::RenameSymbols renamer;
  for (auto symbol : symbols) {
    auto new_label = factory.translate_label(symbol.label_base, this);

    if (new_label == symbol.label) {
      continue;
    }

    renamer.add_translation(symbol.label, new_label);
  }

  if (renamer.get_translations().size() == 0) {
    return;
  }

  factory.translate(this, this, renamer);
}

std::string Node::process_call_path_filename(std::string call_path_filename) {
  std::string dir_delim = "/";
  std::string ext_delim = ".";

  auto dir_found = call_path_filename.find_last_of(dir_delim);
  if (dir_found != std::string::npos) {
    call_path_filename =
        call_path_filename.substr(dir_found + 1, call_path_filename.size());
  }

  auto ext_found = call_path_filename.find_last_of(ext_delim);
  if (ext_found != std::string::npos) {
    call_path_filename = call_path_filename.substr(0, ext_found);
  }

  return call_path_filename;
}

std::string Node::dump_recursive(int lvl) const {
  std::stringstream result;

  auto pad = std::string(lvl * 2, ' ');

  result << pad << dump(true) << "\n";

  if (next) {
    result << next->dump_recursive(lvl + 1);
  }

  return result.str();
}

klee::ConstraintManager Node::get_constraints() const {
  klee::ConstraintManager accumulated;

  const Node *node = this;

  for (auto c : node->get_node_constraints()) {
    accumulated.addConstraint(c);
  }

  const Node* prev = node->get_prev().get();

  while (prev) {
    if (prev->get_type() == NodeType::BRANCH) {
      auto prev_branch = static_cast<const Branch *>(prev);

      auto on_true = prev_branch->get_on_true();
      auto on_false = prev_branch->get_on_false();

      assert(on_true);
      assert(on_false);

      auto condition = prev_branch->get_condition();
      assert(!condition.isNull());
      
      if (on_true->get_id() == node->get_id()) {
        accumulated.addConstraint(condition);
      } else {
        assert(on_false->get_id() == node->get_id());
        auto not_condition = kutil::solver_toolbox.exprBuilder->Not(condition);
        accumulated.addConstraint(not_condition);
      }
    }

    for (auto c : prev->get_node_constraints()) {
      accumulated.addConstraint(c);
    }

    node = prev;
    prev = prev->get_prev().get();
  }

  return accumulated;
}

} // namespace BDD