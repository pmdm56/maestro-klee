#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class x86TofinoMemoryBank : public MemoryBank {
private:
  BDD::symbol_t time;

public:
  x86TofinoMemoryBank() : MemoryBank() {}
  x86TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  void set_time(BDD::symbol_t _time) {
    time = _time;
  }

  BDD::symbol_t get_time() const { return time; }

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new x86TofinoMemoryBank());
  }
};

} // namespace x86_tofino
} // namespace targets
} // namespace synapse