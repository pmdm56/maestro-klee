#pragma once

#include "../module.h"
#include "ignore.h"

namespace synapse {
namespace targets {
namespace tofino {

class RegisterRead : public Module {
public:
  RegisterRead()
      : Module(ModuleType::Tofino_RegisterRead, TargetType::Tofino,
               "RegisterRead") {}

  RegisterRead(BDD::BDDNode_ptr node)
      : Module(ModuleType::Tofino_RegisterRead, TargetType::Tofino,
               "RegisterRead", node) {}

private:
  bool process(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
               const BDD::Call *casted, processing_result_t &result) {
    return false;
    // TODO: implement
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    process(ep, node, casted, result);
    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new RegisterRead(node);
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
