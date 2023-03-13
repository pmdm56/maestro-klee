#pragma once

#include "../../target.h"
#include "../module.h"

#include "current_time.h"
#include "get_packet_hdr.h"
#include "memory_bank.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class x86TofinoTarget : public Target {
public:
  x86TofinoTarget()
      : Target(TargetType::x86_Tofino,
               {
                   MODULE(CurrentTime),
                   MODULE(GetPacketHeader),
               },
               MemoryBank_ptr(new x86TofinoMemoryBank())) {}

  static Target_ptr build() { return Target_ptr(new x86TofinoTarget()); }
};

} // namespace x86_tofino
} // namespace targets
} // namespace synapse
