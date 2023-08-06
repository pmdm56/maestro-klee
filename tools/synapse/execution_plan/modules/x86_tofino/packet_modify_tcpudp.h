#pragma once

#include "../module.h"
#include "ignore.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class PacketModifyTCPUDP : public Module {
private:
  klee::ref<klee::Expr> tcpudp_chunk;
  std::vector<modification_t> modifications;

public:
  PacketModifyTCPUDP()
      : Module(ModuleType::x86_Tofino_PacketModifyTCPUDP,
               TargetType::x86_Tofino, "PacketModifyTCPUDP") {}

  PacketModifyTCPUDP(BDD::Node_ptr node, klee::ref<klee::Expr> _tcpudp_chunk,
                     const std::vector<modification_t> &_modifications)
      : Module(ModuleType::x86_Tofino_PacketModifyTCPUDP,
               TargetType::x86_Tofino, "PacketModifyTCPUDP", node),
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

  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name != symbex::FN_RETURN_CHUNK) {
      return result;
    }

    auto all_prev_packet_borrow_next_chunk =
        get_prev_fn(ep, node, symbex::FN_BORROW_CHUNK);

    assert(all_prev_packet_borrow_next_chunk.size());

    auto all_prev_packet_return_chunk =
        get_prev_fn(ep, node, symbex::FN_RETURN_CHUNK);

    if (all_prev_packet_return_chunk.size() != 0 ||
        all_prev_packet_borrow_next_chunk.size() < 3 ||
        is_ip_options(all_prev_packet_borrow_next_chunk[0].get())) {
      return result;
    }

    auto borrow_tcpudp = all_prev_packet_borrow_next_chunk[0].get();

    auto curr_tcpudp_chunk = call.args[symbex::FN_BORROW_CHUNK_EXTRA].in;
    auto prev_tcpudp_chunk = get_tcpudp_chunk(borrow_tcpudp);

    if (curr_tcpudp_chunk->getWidth() != 4 * 8 ||
        prev_tcpudp_chunk->getWidth() != 4 * 8) {
      return result;
    }

    auto _modifications =
        build_modifications(prev_tcpudp_chunk, curr_tcpudp_chunk);

    if (_modifications.size() == 0) {
      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::x86_Tofino);

      result.module = new_module;
      result.next_eps.push_back(new_ep);

      return result;
    }

    auto new_module = std::make_shared<PacketModifyTCPUDP>(
        node, prev_tcpudp_chunk, _modifications);
    auto new_ep = ep.add_leaves(new_module, node->get_next());

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const override {
    visitor.visit(ep_node, this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new PacketModifyTCPUDP(node, tcpudp_chunk, modifications);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const PacketModifyTCPUDP *>(other);

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
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
