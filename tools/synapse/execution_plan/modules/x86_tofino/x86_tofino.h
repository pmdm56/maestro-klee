#pragma once

#include "../../target.h"
#include "../module.h"

#include "current_time.h"
#include "drop.h"
#include "else.h"
#include "if.h"
#include "ignore.h"
#include "memory_bank.h"
#include "packet_parse_cpu.h"
#include "packet_parse_ethernet.h"
#include "then.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class x86TofinoTarget : public Target {
public:
  x86TofinoTarget()
      : Target(TargetType::x86_Tofino,
               {
                   MODULE(Ignore),
                   MODULE(CurrentTime),
                   MODULE(PacketParseCPU),
                   MODULE(PacketParseEthernet),
                   MODULE(If),
                   MODULE(Then),
                   MODULE(Else),
                   MODULE(Drop),
               },
               MemoryBank_ptr(new x86TofinoMemoryBank())) {}

  static Target_ptr build() { return Target_ptr(new x86TofinoTarget()); }
};

} // namespace x86_tofino
} // namespace targets
} // namespace synapse
