#pragma once

#include "../module.h"

#include "ethernet_consume.h"

namespace synapse {
namespace targets {
namespace tofino {

inline std::vector<Module_ptr> get_modules() {
  std::vector<Module_ptr> modules{
      MODULE(EthernetConsume),
  };

  return modules;
}
} // namespace tofino
} // namespace targets
} // namespace synapse
