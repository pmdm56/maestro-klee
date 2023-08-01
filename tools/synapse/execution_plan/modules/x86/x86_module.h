#pragma once

#include "../module.h"
#include "data_structures/data_structures.h"
#include "memory_bank.h"

namespace synapse {
namespace targets {
namespace x86 {

class x86Module : public Module {
public:
  x86Module(ModuleType _type, const char *_name)
      : Module(_type, TargetType::x86, _name) {}

  x86Module(ModuleType _type, const char *_name, BDD::Node_ptr node)
      : Module(_type, TargetType::x86, _name, node) {}

protected:
  void save_map(const ExecutionPlan &ep, addr_t addr) {
    auto mb = ep.get_memory_bank<x86MemoryBank>(TargetType::x86);
    auto saved = mb->has_map_config(addr);
    if (!saved) {
      auto cfg = BDD::symbex::get_map_config(ep.get_bdd(), addr);
      mb->save_map_config(addr, cfg);
    }
  }

  void save_vector(const ExecutionPlan &ep, addr_t addr) {
    auto mb = ep.get_memory_bank<x86MemoryBank>(TargetType::x86);
    auto saved = mb->has_vector_config(addr);
    if (!saved) {
      auto cfg = BDD::symbex::get_vector_config(ep.get_bdd(), addr);
      mb->save_vector_config(addr, cfg);
    }
  }

  void save_dchain(const ExecutionPlan &ep, addr_t addr) {
    auto mb = ep.get_memory_bank<x86MemoryBank>(TargetType::x86);
    auto saved = mb->has_dchain_config(addr);
    if (!saved) {
      auto cfg = BDD::symbex::get_dchain_config(ep.get_bdd(), addr);
      mb->save_dchain_config(addr, cfg);
    }
  }

  void save_sketch(const ExecutionPlan &ep, addr_t addr) {
    auto mb = ep.get_memory_bank<x86MemoryBank>(TargetType::x86);
    auto saved = mb->has_sketch_config(addr);
    if (!saved) {
      auto cfg = BDD::symbex::get_sketch_config(ep.get_bdd(), addr);
      mb->save_sketch_config(addr, cfg);
    }
  }

  void save_cht(const ExecutionPlan &ep, addr_t addr) {
    auto mb = ep.get_memory_bank<x86MemoryBank>(TargetType::x86);
    auto saved = mb->has_cht_config(addr);
    if (!saved) {
      auto cfg = BDD::symbex::get_cht_config(ep.get_bdd(), addr);
      mb->save_cht_config(addr, cfg);
    }
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const = 0;
  virtual Module_ptr clone() const = 0;
  virtual bool equals(const Module *other) const = 0;
};

} // namespace x86
} // namespace targets
} // namespace synapse
