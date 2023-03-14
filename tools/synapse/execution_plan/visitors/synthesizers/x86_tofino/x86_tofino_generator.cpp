#include "x86_tofino_generator.h"

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

void x86TofinoGenerator::visit(ExecutionPlan ep) { assert(false && "TODO"); }

void x86TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  assert(false && "TODO");
}

void x86TofinoGenerator::visit(const targets::x86_tofino::CurrentTime *node) {
  assert(false && "TODO");
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::PacketParseCPU *node) {
  assert(false && "TODO");
}

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse