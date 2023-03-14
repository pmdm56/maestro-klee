#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace bmv2 {

class BMv2MemoryBank : public MemoryBank {
public:
  BMv2MemoryBank() : MemoryBank() {}
  BMv2MemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new BMv2MemoryBank());
  }
};

} // namespace bmv2
} // namespace targets
} // namespace synapse