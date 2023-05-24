#include "memory_bank.h"
#include "../module.h"

namespace synapse {
namespace targets {
namespace tofino {

void TofinoMemoryBank::postpone(BDD::node_id_t node_id, Module_ptr module) {
  postponed.emplace_back(node_id, module);
}

const std::vector<postponed_t> &TofinoMemoryBank::get_postponed() const {
  return postponed;
}

} // namespace tofino
} // namespace targets
} // namespace synapse