#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace fpga {

class FPGAMemoryBank : public MemoryBank {
public:
  FPGAMemoryBank() : MemoryBank() {}
  FPGAMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new FPGAMemoryBank());
  }
};

} // namespace fpga
} // namespace targets
} // namespace synapse