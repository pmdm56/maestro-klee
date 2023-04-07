#pragma once

#include <klee/Expr.h>

#include <string>
#include <unordered_map>

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace tofino {

class TofinoMemoryBank : public MemoryBank {
private:
  std::unordered_map<obj_addr_t, std::string> obj_addr_to_table_name;

public:
  TofinoMemoryBank() : MemoryBank() {}
  TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}
  TofinoMemoryBank(const TofinoMemoryBank &mb)
      : MemoryBank(mb), obj_addr_to_table_name(mb.obj_addr_to_table_name) {}

  void save_obj_addr_to_table_name(klee::ref<klee::Expr> expr_addr,
                                   const std::string &table_name) {
    auto obj_addr = expr_addr_to_obj_addr(expr_addr);
    obj_addr_to_table_name[obj_addr] = table_name;
  }

  bool has_obj_addr_to_table_name(klee::ref<klee::Expr> expr_addr) const {
    auto obj_addr = expr_addr_to_obj_addr(expr_addr);
    return obj_addr_to_table_name.find(obj_addr) !=
           obj_addr_to_table_name.end();
  }

  const std::string &
  get_obj_addr_to_table_name(klee::ref<klee::Expr> expr_addr) const {
    assert(has_obj_addr_to_table_name(expr_addr));
    auto obj_addr = expr_addr_to_obj_addr(expr_addr);
    return obj_addr_to_table_name.at(obj_addr);
  }

  virtual MemoryBank_ptr clone() const {
    auto clone = new TofinoMemoryBank(*this);
    return MemoryBank_ptr(clone);
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse