#pragma once

#include "../../memory_bank.h"

namespace synapse {
namespace targets {
namespace netronome {

class NetronomeMemoryBank : public MemoryBank {
public:
  NetronomeMemoryBank() : MemoryBank() {}
  NetronomeMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new NetronomeMemoryBank());
  }
};

} // namespace netronome
} // namespace targets
} // namespace synapse