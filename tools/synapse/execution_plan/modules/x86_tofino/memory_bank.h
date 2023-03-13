#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class x86TofinoMemoryBank : public MemoryBank {
public:
  x86TofinoMemoryBank() : MemoryBank() {}
  x86TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new x86TofinoMemoryBank());
  }
};

} // namespace x86_tofino
} // namespace targets
} // namespace synapse