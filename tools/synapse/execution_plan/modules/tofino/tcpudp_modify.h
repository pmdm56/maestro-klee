#pragma once

#include "../module.h"
#include "ignore.h"

namespace synapse {
namespace targets {
namespace tofino {

class TCPUDPModify : public Module {
private:
  klee::ref<klee::Expr> tcpudp_chunk;
  std::vector<modification_t> modifications;

public:
  TCPUDPModify()
      : Module(ModuleType::Tofino_TCPUDPModify, TargetType::Tofino,
               "TCPUDPModify") {}

  TCPUDPModify(BDD::Node_ptr node, klee::ref<klee::Expr> _tcpudp_chunk,
               const std::vector<modification_t> &_modifications)
      : Module(ModuleType::Tofino_TCPUDPModify, TargetType::Tofino,
               "TCPUDPModify", node),
        tcpudp_chunk(_tcpudp_chunk), modifications(_modifications) {}

private:
  klee::ref<klee::Expr> get_tcpudp_chunk(const BDD::Node *node) const {
    assert(node->get_type() == BDD::Node::NodeType::CALL);

    auto call_node = static_cast<const BDD::Call *>(node);
    auto call = call_node->get_call();

    assert(call.function_name == symbex::FN_BORROW_CHUNK);
    assert(!call.extra_vars[symbex::FN_BORROW_CHUNK_EXTRA].second.isNull());

    return call.extra_vars[symbex::FN_BORROW_CHUNK_EXTRA].second;
  }

  bool is_ip_options(const BDD::Node *node) const {
    assert(node->get_type() == BDD::Node::NodeType::CALL);

    auto call_node = static_cast<const BDD::Call *>(node);
    auto call = call_node->get_call();

    auto len = call.args[symbex::FN_BORROW_CHUNK_ARG_LEN].expr;
    return len->getKind() != klee::Expr::Kind::Constant;
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::Node_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    if (call.function_name != symbex::FN_RETURN_CHUNK) {
      return result;
    }

    auto all_prev_packet_borrow_next_chunk =
        get_all_prev_functions(ep, node, symbex::FN_BORROW_CHUNK);

    assert(all_prev_packet_borrow_next_chunk.size());

    auto all_prev_packet_return_chunk =
        get_all_prev_functions(ep, node, symbex::FN_RETURN_CHUNK);

    if (all_prev_packet_return_chunk.size() != 0 ||
        all_prev_packet_borrow_next_chunk.size() < 3 ||
        is_ip_options(all_prev_packet_borrow_next_chunk[0].get())) {
      return result;
    }

    auto borrow_tcpudp = all_prev_packet_borrow_next_chunk[0].get();

    auto curr_tcpudp_chunk = call.args[symbex::FN_BORROW_CHUNK_EXTRA].in;
    auto prev_tcpudp_chunk = get_tcpudp_chunk(borrow_tcpudp);

    assert(curr_tcpudp_chunk->getWidth() == 4 * 8);
    assert(prev_tcpudp_chunk->getWidth() == 4 * 8);

    auto _modifications =
        build_modifications(prev_tcpudp_chunk, curr_tcpudp_chunk);

    if (_modifications.size() == 0) {
      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::Tofino);

      result.module = new_module;
      result.next_eps.push_back(new_ep);

      return result;
    }

    auto new_module =
        std::make_shared<TCPUDPModify>(node, prev_tcpudp_chunk, _modifications);
    auto new_ep = ep.add_leaves(new_module, node->get_next());

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new TCPUDPModify(node, tcpudp_chunk, modifications);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const TCPUDPModify *>(other);

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            tcpudp_chunk, other_cast->tcpudp_chunk)) {
      return false;
    }

    auto other_modifications = other_cast->get_modifications();

    if (modifications.size() != other_modifications.size()) {
      return false;
    }

    for (unsigned i = 0; i < modifications.size(); i++) {
      auto modification = modifications[i];
      auto other_modification = other_modifications[i];

      if (modification.byte != other_modification.byte) {
        return false;
      }

      if (!kutil::solver_toolbox.are_exprs_always_equal(
              modification.expr, other_modification.expr)) {
        return false;
      }
    }

    return true;
  }

  klee::ref<klee::Expr> get_tcpudp_chunk() const { return tcpudp_chunk; }

  const std::vector<modification_t> &get_modifications() const {
    return modifications;
  }
};
} // namespace tofino
} // namespace targets
} // namespace synapse
