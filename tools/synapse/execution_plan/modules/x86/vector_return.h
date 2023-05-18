#pragma once

#include "x86_module.h"

namespace synapse {
namespace targets {
namespace x86 {

class VectorReturn : public x86Module {
private:
  obj_addr_t vector_addr;
  klee::ref<klee::Expr> index;
  obj_addr_t value_addr;
  klee::ref<klee::Expr> value;

public:
  VectorReturn() : x86Module(ModuleType::x86_VectorReturn, "VectorReturn") {}

  VectorReturn(BDD::Node_ptr node, obj_addr_t _vector_addr,
               klee::ref<klee::Expr> _index, obj_addr_t _value_addr,
               klee::ref<klee::Expr> _value)
      : x86Module(ModuleType::x86_VectorReturn, "VectorReturn", node),
        vector_addr(_vector_addr), index(_index), value_addr(_value_addr),
        value(_value) {}

private:
  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name == symbex::FN_VECTOR_RETURN) {
      assert(!call.args[symbex::FN_VECTOR_ARG_VECTOR].expr.isNull());
      assert(!call.args[symbex::FN_VECTOR_ARG_INDEX].expr.isNull());
      assert(!call.args[symbex::FN_VECTOR_ARG_VALUE].expr.isNull());
      assert(!call.args[symbex::FN_VECTOR_ARG_VALUE].in.isNull());

      auto _vector = call.args[symbex::FN_VECTOR_ARG_VECTOR].expr;
      auto _index = call.args[symbex::FN_VECTOR_ARG_INDEX].expr;
      auto _value_addr_expr = call.args[symbex::FN_VECTOR_ARG_VALUE].expr;
      auto _value = call.args[symbex::FN_VECTOR_ARG_VALUE].in;

      auto _vector_addr = kutil::expr_addr_to_obj_addr(_vector);
      auto _value_addr = kutil::expr_addr_to_obj_addr(_value_addr_expr);

      save_vector(ep, _vector_addr);

      auto new_module = std::make_shared<VectorReturn>(
          node, _vector_addr, _index, _value_addr, _value);
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
    auto cloned = new VectorReturn(node, vector_addr, index, value_addr, value);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const VectorReturn *>(other);

    if (vector_addr != other_cast->get_vector_addr()) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            index, other_cast->get_index())) {
      return false;
    }

    if (value_addr != other_cast->get_value_addr()) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            value, other_cast->get_value())) {
      return false;
    }

    return true;
  }

  obj_addr_t get_vector_addr() const { return vector_addr; }
  const klee::ref<klee::Expr> &get_index() const { return index; }
  obj_addr_t get_value_addr() const { return value_addr; }
  const klee::ref<klee::Expr> &get_value() const { return value; }
};
} // namespace x86
} // namespace targets
} // namespace synapse
