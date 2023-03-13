#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class PacketParseCPU : public Module {
public:
  PacketParseCPU()
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU") {}

  PacketParseCPU(BDD::BDDNode_ptr node)
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU", node) {}

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new PacketParseCPU(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
