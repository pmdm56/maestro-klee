#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace x86_bmv2 {

class x86BMv2MemoryBank : public MemoryBank {
public:
  x86BMv2MemoryBank() : MemoryBank() {}
  x86BMv2MemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new x86BMv2MemoryBank());
  }
};

} // namespace x86_bmv2
} // namespace targets
} // namespace synapse