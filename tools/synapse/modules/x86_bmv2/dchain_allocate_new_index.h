#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_bmv2 {

class DchainAllocateNewIndex : public Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> success;

  BDD::symbols_t generated_symbols;

public:
  DchainAllocateNewIndex()
      : Module(ModuleType::x86_BMv2_DchainAllocateNewIndex, Target::x86_BMv2,
               "DchainAllocate") {}

  DchainAllocateNewIndex(BDD::BDDNode_ptr node,
                         klee::ref<klee::Expr> _dchain_addr,
                         klee::ref<klee::Expr> _time,
                         klee::ref<klee::Expr> _index_out,
                         klee::ref<klee::Expr> _success,
                         BDD::symbols_t _generated_symbols)
      : Module(ModuleType::x86_BMv2_DchainAllocateNewIndex, Target::x86_BMv2,
               "DchainAllocate", node),
        dchain_addr(_dchain_addr), time(_time), index_out(_index_out),
        success(_success), generated_symbols(_generated_symbols) {}

private:
  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    if (call.function_name == symbex::FN_DCHAIN_ALLOCATE) {
      assert(!call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr.isNull());
      assert(!call.args[symbex::FN_DCHAIN_ARG_TIME].expr.isNull());
      assert(!call.args[symbex::FN_DCHAIN_ARG_OUT].out.isNull());
      assert(!call.ret.isNull());

      auto _dchain_addr = call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr;
      auto _time = call.args[symbex::FN_DCHAIN_ARG_TIME].expr;
      auto _index_out = call.args[symbex::FN_DCHAIN_ARG_OUT].out;
      auto _success = call.ret;

      auto _generated_symbols = casted->get_generated_symbols();

      auto new_module = std::make_shared<DchainAllocateNewIndex>(
          node, _dchain_addr, _time, _index_out, _success, _generated_symbols);
      auto new_ep = ep.add_leaves(new_module, node->get_next());

      result.module = new_module;
      result.next_eps.push_back(new_ep);
    }

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new DchainAllocateNewIndex(node, dchain_addr, time, index_out,
                                             success, generated_symbols);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const DchainAllocateNewIndex *>(other);

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            dchain_addr, other_cast->get_dchain_addr())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(time,
                                                      other_cast->get_time())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            index_out, other_cast->get_index_out())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            success, other_cast->get_success())) {
      return false;
    }

    if (generated_symbols != other_cast->generated_symbols) {
      return false;
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_dchain_addr() const { return dchain_addr; }
  const klee::ref<klee::Expr> &get_time() const { return time; }
  const klee::ref<klee::Expr> &get_index_out() const { return index_out; }
  const klee::ref<klee::Expr> &get_success() const { return success; }

  const BDD::symbols_t &get_generated_symbols() const {
    return generated_symbols;
  }
};
} // namespace x86_bmv2
} // namespace targets
} // namespace synapse
