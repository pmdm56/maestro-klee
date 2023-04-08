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
  std::unordered_map<obj_addr_t, std::unordered_set<std::string>>
      obj_addr_to_table_names;

public:
  TofinoMemoryBank() : MemoryBank() {}
  TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}
  TofinoMemoryBank(const TofinoMemoryBank &mb)
      : MemoryBank(mb), obj_addr_to_table_names(mb.obj_addr_to_table_names) {}

  void save_obj_addr_table_name(klee::ref<klee::Expr> expr_addr,
                                const std::string &table_name) {
    auto obj_addr = expr_addr_to_obj_addr(expr_addr);
    obj_addr_to_table_names[obj_addr].insert(table_name);
  }

  bool does_obj_have_table_names(klee::ref<klee::Expr> expr_addr) const {
    auto obj_addr = expr_addr_to_obj_addr(expr_addr);
    auto found_it = obj_addr_to_table_names.find(obj_addr);

    return found_it != obj_addr_to_table_names.end() &&
           found_it->second.size() > 0;
  }

  std::unordered_set<std::string>
  get_table_names_of_obj(klee::ref<klee::Expr> expr_addr) const {
    assert(does_obj_have_table_names(expr_addr));
    auto obj_addr = expr_addr_to_obj_addr(expr_addr);
    return obj_addr_to_table_names.at(obj_addr);
  }

  virtual MemoryBank_ptr clone() const {
    auto clone = new TofinoMemoryBank(*this);
    return MemoryBank_ptr(clone);
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse