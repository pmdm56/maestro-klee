#pragma once

#include "../module.h"
#include "ignore.h"

namespace synapse {
namespace targets {
namespace tofino {

class SetupExpirationNotifications : public Module {
public:
  SetupExpirationNotifications()
      : Module(ModuleType::Tofino_SetupExpirationNotifications,
               TargetType::Tofino, "SetupExpirationNotifications") {}

  SetupExpirationNotifications(BDD::BDDNode_ptr node)
      : Module(ModuleType::Tofino_SetupExpirationNotifications,
               TargetType::Tofino, "SetupExpirationNotifications", node) {}

private:
  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    if (call.function_name != symbex::FN_EXPIRE_MAP) {
      return result;
    }

    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_CHAIN].expr.isNull());
    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_VECTOR].expr.isNull());
    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_MAP].expr.isNull());
    assert(!call.args[symbex::FN_EXPIRE_MAP_ARG_TIME].expr.isNull());
    assert(!call.ret.isNull());

    auto _dchain_addr = call.args[symbex::FN_EXPIRE_MAP_ARG_CHAIN].expr;
    auto _vector_addr = call.args[symbex::FN_EXPIRE_MAP_ARG_VECTOR].expr;
    auto _map_addr = call.args[symbex::FN_EXPIRE_MAP_ARG_MAP].expr;
    auto _time = call.args[symbex::FN_EXPIRE_MAP_ARG_TIME].expr;
    auto _number_of_freed_flows = call.ret;

    // we should ignore this, but remember the expiration setup
    // TODO:

    auto new_module = std::make_shared<Ignore>(node);
    auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::Tofino);

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new SetupExpirationNotifications(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    return true;
  }
};
} // namespace tofino
} // namespace targets
} // namespace synapse
