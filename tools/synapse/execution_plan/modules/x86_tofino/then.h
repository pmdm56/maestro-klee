#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class Then : public Module {
public:
  Then() : Module(ModuleType::x86_Tofino_Then, TargetType::x86_Tofino, "Then") {}

  Then(BDD::Node_ptr node)
      : Module(ModuleType::x86_Tofino_Then, TargetType::x86_Tofino, "Then", node) {}

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new Then(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace tofino
} // namespace targets
} // namespace synapse
