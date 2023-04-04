#pragma once

#include "../module.h"
#include "memory_bank.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class Ignore : public Module {
private:
  std::vector<std::string> functions_to_ignore;

public:
  Ignore()
      : Module(ModuleType::x86_Tofino_Ignore, TargetType::x86_Tofino,
               "Ignore") {
    functions_to_ignore = std::vector<std::string>{
        symbex::FN_CURRENT_TIME,
        symbex::FN_EXPIRE_MAP,
        symbex::FN_ETHER_HASH,
    };
  }

  Ignore(BDD::BDDNode_ptr node)
      : Module(ModuleType::x86_Tofino_Ignore, TargetType::x86_Tofino, "Ignore",
               node) {}

private:
  void process_current_time(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
                            const BDD::Call *casted) {
    auto call = casted->get_call();
    assert(!call.ret.isNull());

    auto _time = call.ret;
    auto _generated_symbols = casted->get_local_generated_symbols();
    assert(_generated_symbols.size() == 1);

    auto time_symbol = *_generated_symbols.begin();

    auto mb = ep.get_memory_bank<x86TofinoMemoryBank>(x86_Tofino);
    mb->set_time(time_symbol);
  }

  void process_expire_items_single_map(const ExecutionPlan &ep,
                                       BDD::BDDNode_ptr node,
                                       const BDD::Call *casted) {
    auto call = casted->get_call();

    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_CHAIN].expr.isNull());
    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_MAP].expr.isNull());
    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_VECTOR].expr.isNull());
    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_TIME].expr.isNull());

    auto _chain = call.args[symbex::FN_EXPIRE_MAP_ARG_CHAIN].expr;
    auto _map = call.args[symbex::FN_EXPIRE_MAP_ARG_MAP].expr;
    auto _vector = call.args[symbex::FN_EXPIRE_MAP_ARG_VECTOR].expr;
    auto _time = call.args[symbex::FN_EXPIRE_MAP_ARG_TIME].expr;

    auto expiration =
        x86TofinoMemoryBank::expiration_t(_chain, _map, _vector, _time);

    auto mb = ep.get_memory_bank<x86TofinoMemoryBank>(x86_Tofino);
    mb->set_expiration(expiration);
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    auto found_it = std::find(functions_to_ignore.begin(),
                              functions_to_ignore.end(), call.function_name);

    if (call.function_name == symbex::FN_CURRENT_TIME) {
      process_current_time(ep, node, casted);
    } else if (call.function_name == symbex::FN_EXPIRE_MAP) {
      process_expire_items_single_map(ep, node, casted);
    }

    if (found_it != functions_to_ignore.end()) {
      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::x86_Tofino);

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
    auto cloned = new Ignore(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
