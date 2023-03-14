#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace bmv2 {

class Else : public Module {
public:
  Else()
      : Module(ModuleType::BMv2_Else,
               TargetType::BMv2, "Else") {}
  Else(BDD::BDDNode_ptr node)
      : Module(ModuleType::BMv2_Else,
               TargetType::BMv2, "Else", node) {}

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new Else(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace bmv2
} // namespace targets
} // namespace synapse
