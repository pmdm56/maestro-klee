#pragma once

#include "../../execution_plan.h"

namespace synapse {
namespace synthesizer {

bool pending_packet_borrow_next_chunk(const ExecutionPlanNode *ep_node,
                                      synapse::Target target);

}
} // namespace synapse