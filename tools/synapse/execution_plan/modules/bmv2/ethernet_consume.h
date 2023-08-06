#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace bmv2 {

class EthernetConsume : public Module {
private:
  klee::ref<klee::Expr> chunk;

public:
  EthernetConsume()
      : Module(ModuleType::BMv2_EthernetConsume, TargetType::BMv2,
               "EthernetConsume") {}

  EthernetConsume(BDD::Node_ptr node, klee::ref<klee::Expr> _chunk)
      : Module(ModuleType::BMv2_EthernetConsume, TargetType::BMv2,
               "EthernetConsume", node),
        chunk(_chunk) {}

private:
  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name != symbex::FN_BORROW_CHUNK) {
      return result;
    }

    auto all_prev_packet_borrow_next_chunk =
        get_prev_fn(ep, node, symbex::FN_BORROW_CHUNK);

    if (all_prev_packet_borrow_next_chunk.size() != 0) {
      return result;
    }

    assert(!call.args[symbex::FN_BORROW_CHUNK_ARG_LEN].expr.isNull());
    assert(!call.extra_vars[symbex::FN_BORROW_CHUNK_EXTRA].second.isNull());

    auto _length = call.args[symbex::FN_BORROW_CHUNK_ARG_LEN].expr;
    auto _chunk = call.extra_vars[symbex::FN_BORROW_CHUNK_EXTRA].second;

    // Make sure that packet_borrow_next_chunk borrows the
    // 14 ethernet bytes
    assert(_length->getKind() == klee::Expr::Kind::Constant);
    assert(kutil::solver_toolbox.value_from_expr(_length) == 14);

    auto new_module = std::make_shared<EthernetConsume>(node, _chunk);
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
    auto cloned = new EthernetConsume(node, chunk);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }

  const klee::ref<klee::Expr> &get_chunk() const { return chunk; }
};
} // namespace bmv2
} // namespace targets
} // namespace synapse
