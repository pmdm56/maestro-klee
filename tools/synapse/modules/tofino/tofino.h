#pragma once

#include "../module.h"

#include "drop.h"
#include "else.h"
#include "ethernet_consume.h"
#include "ethernet_modify.h"
#include "forward.h"
#include "if.h"
#include "ignore.h"
#include "send_to_controller.h"
#include "table_lookup.h"
#include "then.h"

namespace synapse {
namespace targets {
namespace tofino {

inline std::vector<Module_ptr> get_modules() {
  std::vector<Module_ptr> modules{
      MODULE(Ignore),         MODULE(If),
      MODULE(Then),           MODULE(Else),
      MODULE(Forward),        MODULE(EthernetConsume),
      MODULE(EthernetModify), MODULE(TableLookup),
      MODULE(Drop),           MODULE(SendToController),
  };

  return modules;
}
} // namespace tofino
} // namespace targets
} // namespace synapse
