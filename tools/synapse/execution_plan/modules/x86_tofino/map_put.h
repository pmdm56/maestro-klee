#pragma once

#include "../module.h"
#include "ignore.h"
#include "memory_bank.h"

#include "klee-util.h" //FIXME: remove this

namespace synapse {
namespace targets {
namespace x86_tofino {

class MapPut : public Module {
private:
  klee::ref<klee::Expr> map_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

public:
  MapPut()
      : Module(ModuleType::x86_Tofino_MapPut, TargetType::x86_Tofino,
               "MapPut") {}

  MapPut(BDD::BDDNode_ptr node, klee::ref<klee::Expr> _map_addr,
         klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value)
      : Module(ModuleType::x86_Tofino_MapPut, TargetType::x86_Tofino, "MapPut",
               node),
        map_addr(_map_addr), key(_key), value(_value) {}

private:
  processing_result_t process_map_put(const ExecutionPlan &ep,
                                      BDD::BDDNode_ptr node,
                                      const BDD::Call *casted) {
    processing_result_t result;
    auto call = casted->get_call();

    assert(!call.args[symbex::FN_MAP_ARG_MAP].expr.isNull());
    assert(!call.args[symbex::FN_MAP_ARG_KEY].in.isNull());
    assert(!call.args[symbex::FN_MAP_ARG_VALUE].expr.isNull());

    auto _map_addr = call.args[symbex::FN_MAP_ARG_MAP].expr;
    auto _key = call.args[symbex::FN_MAP_ARG_KEY].in;
    auto _value = call.args[symbex::FN_MAP_ARG_VALUE].expr;

    auto new_module = std::make_shared<MapPut>(node, _map_addr, _key, _value);
    auto new_ep = ep.add_leaves(new_module, node->get_next());

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return result;
  }

  processing_result_t process_vector_return(const ExecutionPlan &ep,
                                            BDD::BDDNode_ptr node,
                                            const BDD::Call *casted) {
    processing_result_t result;
    auto call = casted->get_call();

    assert(!call.args[symbex::FN_VECTOR_ARG_VECTOR].expr.isNull());
    assert(!call.args[symbex::FN_VECTOR_ARG_INDEX].expr.isNull());
    assert(!call.args[symbex::FN_VECTOR_ARG_VALUE].expr.isNull());
    assert(!call.args[symbex::FN_VECTOR_ARG_VALUE].in.isNull());

    auto _vector_addr = call.args[symbex::FN_VECTOR_ARG_VECTOR].expr;
    auto _index = call.args[symbex::FN_VECTOR_ARG_INDEX].expr;
    auto _value_addr = call.args[symbex::FN_VECTOR_ARG_VALUE].expr;
    auto _value = call.args[symbex::FN_VECTOR_ARG_VALUE].in;

    auto mb = ep.get_memory_bank<x86TofinoMemoryBank>(x86_Tofino);
    auto last_value = mb->get_vector_borrow_value(_vector_addr);
    assert(!last_value.isNull());

    auto eq = kutil::solver_toolbox.are_exprs_always_equal(last_value, _value);

    if (eq) {
      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::x86_Tofino);

      result.module = new_module;
      result.next_eps.push_back(new_ep);
    } else {
      auto new_module =
          std::make_shared<MapPut>(node, _vector_addr, _index, _value);
      auto new_ep = ep.add_leaves(new_module, node->get_next());

      result.module = new_module;
      result.next_eps.push_back(new_ep);
    }

    return result;
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    if (call.function_name == symbex::FN_MAP_PUT) {
      return process_map_put(ep, node, casted);
    }

    if (call.function_name == symbex::FN_VECTOR_RETURN) {
      return process_vector_return(ep, node, casted);
    }

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new MapPut(node, map_addr, key, value);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const MapPut *>(other);

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            map_addr, other_cast->get_map_addr())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(key,
                                                      other_cast->get_key())) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            value, other_cast->get_value())) {
      return false;
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_map_addr() const { return map_addr; }
  const klee::ref<klee::Expr> &get_key() const { return key; }
  const klee::ref<klee::Expr> &get_value() const { return value; }
};
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
