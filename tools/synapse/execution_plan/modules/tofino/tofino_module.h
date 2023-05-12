#pragma once

#include "../module.h"
#include "data_structures/data_structures.h"
#include "memory_bank.h"

namespace synapse {
namespace targets {
namespace tofino {

class TofinoModule : public Module {
public:
  TofinoModule(ModuleType _type, const char *_name)
      : Module(_type, TargetType::Tofino, _name) {}

  TofinoModule(ModuleType _type, const char *_name, BDD::Node_ptr node)
      : Module(_type, TargetType::Tofino, _name, node) {}

protected:
  

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const = 0;
  virtual Module_ptr clone() const = 0;
  virtual bool equals(const Module *other) const= 0;
};

} // namespace tofino
} // namespace targets
} // namespace synapse
