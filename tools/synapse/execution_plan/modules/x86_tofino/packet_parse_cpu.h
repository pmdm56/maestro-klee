#pragma once

#include "../../../symbex.h"
#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class PacketParseCPU : public Module {
private:
  BDD::symbol_t cpu_code_path;

public:
  PacketParseCPU()
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU") {}

  PacketParseCPU(BDD::BDDNode_ptr node)
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU", node) {
    auto label = symbex::CPU_CODE_PATH;
    auto cpu_code_path_expr =
        kutil::solver_toolbox.create_new_symbol(label, 16);

    cpu_code_path.label = label;
    cpu_code_path.label_base = label;
    cpu_code_path.expr = cpu_code_path_expr;
  }

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

  BDD::symbol_t get_cpu_code_path() const { return cpu_code_path; }
};
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
