#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class Else : public Module {
public:
  Else()
      : Module(ModuleType::x86_Tofino_Else, TargetType::x86_Tofino, "Else") {}
  Else(BDD::BDDNode_ptr node)
      : Module(ModuleType::x86_Tofino_Else, TargetType::x86_Tofino, "Else",
               node) {}

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
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
