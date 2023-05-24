#pragma once

#include <klee/Expr.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../../memory_bank.h"
#include "data_structures/data_structures.h"

namespace synapse {

class Module;
typedef std::shared_ptr<Module> Module_ptr;

namespace targets {
namespace tofino {

typedef std::pair<BDD::node_id_t, Module_ptr> postponed_t;

class TofinoMemoryBank : public MemoryBank {
private:
  DataStructuresSet implementations;
  std::vector<postponed_t> postponed;

public:
  TofinoMemoryBank() : MemoryBank() {}

  TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  TofinoMemoryBank(const TofinoMemoryBank &mb)
      : MemoryBank(mb), implementations(mb.implementations),
        postponed(mb.postponed) {}

  void postpone(BDD::node_id_t node_id, Module_ptr module);
  const std::vector<postponed_t> &get_postponed() const;

  void save_implementation(const DataStructureRef &ds) {
    implementations.insert(ds);
  }

  const std::vector<DataStructureRef> &get_implementations() const {
    return implementations.get();
  }

  std::vector<DataStructureRef>
  get_implementations(DataStructure::Type ds_type) const {
    return implementations.get(ds_type);
  }

  std::vector<DataStructureRef> get_implementations(obj_addr_t obj) const {
    return implementations.get(obj);
  }

  bool check_implementation_compatibility(
      obj_addr_t obj,
      const std::vector<DataStructure::Type> &compatible_types) const {
    DataStructureRef implementation;

    auto this_impls = get_implementations(obj);

    if (this_impls.size() == 0) {
      return true;
    }

    for (auto impl : this_impls) {
      auto found_it = std::find(compatible_types.begin(),
                                compatible_types.end(), impl->get_type());

      if (found_it == compatible_types.end()) {
        return false;
      }
    }

    return true;
  }

  virtual MemoryBank_ptr clone() const {
    auto clone = new TofinoMemoryBank(*this);
    return MemoryBank_ptr(clone);
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse