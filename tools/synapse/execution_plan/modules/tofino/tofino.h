#pragma once

#include "../../target.h"
#include "../module.h"

#include "drop.h"
#include "else.h"
#include "ethernet_consume.h"
#include "ethernet_modify.h"
#include "forward.h"
#include "if.h"
#include "ignore.h"
#include "memory_bank.h"
#include "send_to_controller.h"
#include "table_lookup.h"
#include "then.h"

namespace synapse {
namespace targets {
namespace tofino {

class TofinoTarget : public Target {
public:
  TofinoTarget()
      : Target(TargetType::Tofino,
               {
                   MODULE(Ignore),
                   MODULE(If),
                   MODULE(Then),
                   MODULE(Else),
                   MODULE(Forward),
                   MODULE(EthernetConsume),
                   MODULE(EthernetModify),
                   MODULE(TableLookup),
                   MODULE(Drop),
                   MODULE(SendToController),
               },
               MemoryBank_ptr(new TofinoMemoryBank())) {}

  static Target_ptr build() { return Target_ptr(new TofinoTarget()); }
};

} // namespace tofino
} // namespace targets
} // namespace synapse
