#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"
#include "../synthesizer.h"

#define X86_TOFINO_BOILERPLATE_FILE "boilerplate.cpp"

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

class x86TofinoGenerator : public Synthesizer {
public:
  x86TofinoGenerator()
      : Synthesizer(GET_BOILERPLATE_PATH(X86_TOFINO_BOILERPLATE_FILE)) {}

  virtual void generate(ExecutionPlan &target_ep) override { visit(target_ep); }

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;

  void visit(const targets::x86_tofino::CurrentTime *node) override;
  void visit(const targets::x86_tofino::PacketParseCPU *node) override;
};

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse