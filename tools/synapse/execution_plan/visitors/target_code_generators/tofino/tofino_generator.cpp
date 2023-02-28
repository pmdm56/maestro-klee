#include "tofino_generator.h"

#include "../../../../log.h"
#include "../../../../modules/modules.h"

namespace synapse {

void TofinoGenerator::visit(ExecutionPlan ep) { assert(false && "TODO"); }

void TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  assert(false && "TODO");
}

void TofinoGenerator::visit(const targets::tofino::EthernetConsume *node) {
  assert(false && "TODO");
}

}; // namespace synapse
