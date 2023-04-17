#pragma once

#include <klee/Expr.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace tofino {

class TofinoMemoryBank : public MemoryBank {
private:
  std::unordered_map<obj_addr_t, std::vector<Module_ptr>> tables;

public:
  TofinoMemoryBank() : MemoryBank() {}
  TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}
  TofinoMemoryBank(const TofinoMemoryBank &mb)
      : MemoryBank(mb), tables(mb.tables) {}

  void save_table(obj_addr_t obj_addr, const Module_ptr &table) {
    assert(table->get_type() == Module::Tofino_TableLookup ||
           table->get_type() == Module::Tofino_TableLookupSimple);
    tables[obj_addr].push_back(table);
  }

  bool does_obj_have_tables(obj_addr_t obj_addr) const {
    auto found_it = tables.find(obj_addr);
    return found_it != tables.end() && found_it->second.size() > 0;
  }

  std::vector<Module_ptr> get_obj_tables(obj_addr_t obj_addr) const {
    assert(does_obj_have_tables(obj_addr));
    return tables.at(obj_addr);
  }

  virtual MemoryBank_ptr clone() const {
    auto clone = new TofinoMemoryBank(*this);
    return MemoryBank_ptr(clone);
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse