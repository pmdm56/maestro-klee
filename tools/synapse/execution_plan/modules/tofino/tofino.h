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
#include "int_allocator_allocate.h"
#include "int_allocator_query.h"
#include "int_allocator_rejuvenate.h"
#include "ipv4_consume.h"
#include "ipv4_modify.h"
#include "ipv4_options_consume.h"
#include "ipv4_options_modify.h"
#include "ipv4_tcpudp_checksums_update.h"
#include "memory_bank.h"
#include "modify_custom_header.h"
#include "parse_custom_header.h"
#include "parser_condition.h"
#include "register_read.h"
#include "send_to_controller.h"
#include "setup_expiration_notifications.h"
#include "table_lookup.h"
#include "table_lookup_simple.h"
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
                   MODULE(Then),
                   MODULE(Else),
                   MODULE(Forward),
                   MODULE(ParseCustomHeader),
                   MODULE(ModifyCustomHeader),
                   MODULE(ParserCondition),
                   //  MODULE(IfHeaderValid),
                   //  MODULE(EthernetConsume),
                   //  MODULE(EthernetModify),
                   //  MODULE(IPv4Consume),
                   //  MODULE(IPv4Modify),
                   //  MODULE(IPv4OptionsConsume),
                   //  MODULE(IPv4OptionsModify),
                   //  MODULE(TCPUDPConsume),
                   //  MODULE(TCPUDPModify),
                   //  MODULE(IPv4TCPUDPChecksumsUpdate),
                   MODULE(Drop),
                   MODULE(SendToController),
                   MODULE(SetupExpirationNotifications),
                   MODULE(TableLookup),
                   MODULE(TableLookupSimple),
                   MODULE(RegisterRead),
                   MODULE(IntegerAllocatorAllocate),
                   MODULE(IntegerAllocatorRejuvenate),
                   MODULE(IntegerAllocatorQuery),
               },
               MemoryBank_ptr(new TofinoMemoryBank())) {}

  static Target_ptr build() { return Target_ptr(new TofinoTarget()); }
};

} // namespace tofino
} // namespace targets
} // namespace synapse
