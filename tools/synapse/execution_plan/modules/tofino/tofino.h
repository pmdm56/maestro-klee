#pragma once

#include "../../target.h"
#include "../module.h"

#include "drop.h"
#include "else.h"
#include "ethernet_consume.h"
#include "ethernet_modify.h"
#include "forward.h"
#include "if.h"
#include "if_header_valid.h"
#include "ignore.h"
#include "ipv4_consume.h"
#include "ipv4_modify.h"
#include "ipv4_options_consume.h"
#include "ipv4_options_modify.h"
#include "ipv4_tcpudp_checksums_update.h"
#include "memory_bank.h"
#include "send_to_controller.h"
#include "setup_expiration_notifications.h"
#include "table_lookup.h"
#include "table_lookup_simple.h"
#include "register_read.h"
#include "tcpudp_consume.h"
#include "tcpudp_modify.h"
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
                   MODULE(IfHeaderValid),
                   MODULE(Then),
                   MODULE(Else),
                   MODULE(Forward),
                   MODULE(EthernetConsume),
                   MODULE(EthernetModify),
                   MODULE(IPv4Consume),
                   MODULE(IPv4Modify),
                   MODULE(IPv4OptionsConsume),
                   MODULE(IPv4OptionsModify),
                   MODULE(TCPUDPConsume),
                   MODULE(TCPUDPModify),
                   MODULE(IPv4TCPUDPChecksumsUpdate),
                   MODULE(TableLookup),
                   MODULE(TableLookupSimple),
                   MODULE(RegisterRead),
                   MODULE(Drop),
                   MODULE(SendToController),
                   MODULE(SetupExpirationNotifications),
               },
               MemoryBank_ptr(new TofinoMemoryBank())) {}

  static Target_ptr build() { return Target_ptr(new TofinoTarget()); }
};

} // namespace tofino
} // namespace targets
} // namespace synapse
