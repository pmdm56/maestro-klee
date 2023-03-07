#pragma once

#include "../module.h"
#include "else.h"

namespace synapse {
namespace targets {
namespace bmv2 {

class Then : public Module {
public:
  Then() : Module(ModuleType::BMv2_Then, Target::BMv2, "Then") {}

  Then(BDD::BDDNode_ptr node)
      : Module(ModuleType::BMv2_Then, Target::BMv2, "Then", node) {}

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
} // namespace bmv2
} // namespace targets
} // namespace synapse
