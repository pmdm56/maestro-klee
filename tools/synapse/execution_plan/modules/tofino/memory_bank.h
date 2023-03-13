#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace tofino {

class TofinoMemoryBank : public MemoryBank {
public:
  TofinoMemoryBank() : MemoryBank() {}
  TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new TofinoMemoryBank());
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse