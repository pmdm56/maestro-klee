#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_bmv2 {

class DchainRejuvenateIndex : public Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  DchainRejuvenateIndex()
      : Module(ModuleType::x86_BMv2_DchainRejuvenateIndex, TargetType::x86_BMv2,
               "DchainRejuvenate") {}

  DchainRejuvenateIndex(BDD::Node_ptr node, klee::ref<klee::Expr> _dchain_addr,
                        klee::ref<klee::Expr> _index,
                        klee::ref<klee::Expr> _time)
      : Module(ModuleType::x86_BMv2_DchainRejuvenateIndex, TargetType::x86_BMv2,
               "DchainRejuvenate", node),
        dchain_addr(_dchain_addr), index(_index), time(_time) {}

private:
  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name == BDD::symbex::FN_DCHAIN_REJUVENATE) {
      assert(!call.args[BDD::symbex::FN_DCHAIN_ARG_CHAIN].expr.isNull());
      assert(!call.args[BDD::symbex::FN_DCHAIN_ARG_INDEX].expr.isNull());
      assert(!call.args[BDD::symbex::FN_DCHAIN_ARG_TIME].expr.isNull());

      auto _dchain_addr = call.args[BDD::symbex::FN_DCHAIN_ARG_CHAIN].expr;
      auto _index = call.args[BDD::symbex::FN_DCHAIN_ARG_INDEX].expr;
      auto _time = call.args[BDD::symbex::FN_DCHAIN_ARG_TIME].expr;

      auto new_module = std::make_shared<DchainRejuvenateIndex>(
          node, _dchain_addr, _index, _time);
      auto new_ep = ep.add_leaves(new_module, node->get_next());

      result.module = new_module;
      result.next_eps.push_back(new_ep);
    }

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const override {
    visitor.visit(ep_node, this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new DchainRejuvenateIndex(node, dchain_addr, index, time);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const DchainRejuvenateIndex *>(other);

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            dchain_addr, other_cast->get_dchain_addr())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            index, other_cast->get_index())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(time,
                                                      other_cast->get_time())) {
      return false;
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_dchain_addr() const { return dchain_addr; }
  const klee::ref<klee::Expr> &get_index() const { return index; }
  const klee::ref<klee::Expr> &get_time() const { return time; }
};
} // namespace x86_bmv2
} // namespace targets
} // namespace synapse
