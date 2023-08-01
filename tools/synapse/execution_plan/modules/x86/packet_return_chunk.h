#pragma once

#include "x86_module.h"

namespace synapse {
namespace targets {
namespace x86 {

class PacketReturnChunk : public x86Module {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> original_chunk;
  std::vector<modification_t> modifications;

public:
  PacketReturnChunk()
      : x86Module(ModuleType::x86_PacketReturnChunk, "PacketReturnChunk") {}

  PacketReturnChunk(BDD::Node_ptr node, addr_t _chunk_addr,
                    klee::ref<klee::Expr> _original_chunk,
                    const std::vector<modification_t> &_modifications)
      : x86Module(ModuleType::x86_PacketReturnChunk, "PacketReturnChunk", node),
        chunk_addr(_chunk_addr), original_chunk(_original_chunk),
        modifications(_modifications) {}

private:
  klee::ref<klee::Expr> get_original_chunk(const ExecutionPlan &ep,
                                           BDD::Node_ptr node) const {
    auto prev_borrows = get_prev_fn(ep, node, BDD::symbex::FN_BORROW_CHUNK);
    auto prev_returns = get_prev_fn(ep, node, BDD::symbex::FN_RETURN_CHUNK);

    assert(prev_borrows.size());
    assert(prev_borrows.size() > prev_returns.size());

    auto target = prev_borrows[prev_returns.size()];

    auto call_node = BDD::cast_node<BDD::Call>(target);
    assert(call_node);

    auto call = call_node->get_call();

    assert(call.function_name == BDD::symbex::FN_BORROW_CHUNK);
    assert(!call.extra_vars[BDD::symbex::FN_BORROW_CHUNK_EXTRA].second.isNull());

    return call.extra_vars[BDD::symbex::FN_BORROW_CHUNK_EXTRA].second;
  }

  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name != BDD::symbex::FN_RETURN_CHUNK) {
      return result;
    }

    assert(!call.args[BDD::symbex::FN_BORROW_CHUNK_EXTRA].expr.isNull());
    assert(!call.args[BDD::symbex::FN_BORROW_CHUNK_EXTRA].in.isNull());

    auto _chunk = call.args[BDD::symbex::FN_BORROW_CHUNK_EXTRA].expr;
    auto _current_chunk = call.args[BDD::symbex::FN_BORROW_CHUNK_EXTRA].in;
    auto _original_chunk = get_original_chunk(ep, node);

    auto _chunk_addr = kutil::expr_addr_to_obj_addr(_chunk);
    auto _modifications = build_modifications(_original_chunk, _current_chunk);

    auto new_module = std::make_shared<PacketReturnChunk>(
        node, _chunk_addr, _original_chunk, _modifications);
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
    auto cloned =
        new PacketReturnChunk(node, chunk_addr, original_chunk, modifications);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const PacketReturnChunk *>(other);

    if (chunk_addr != other_cast->get_chunk_addr()) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            original_chunk, other_cast->original_chunk)) {
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

  const addr_t &get_chunk_addr() const { return chunk_addr; }

  klee::ref<klee::Expr> get_original_chunk() const { return original_chunk; }

  const std::vector<modification_t> &get_modifications() const {
    return modifications;
  }
};
} // namespace x86
} // namespace targets
} // namespace synapse
