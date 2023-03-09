#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"

#include "../../visitor.h"
#include "../target_code_generator.h"

#define X86_TOFINO_BOILERPLATE_FILE "boilerplate.c"

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

class x86TofinoGenerator : public Target {
public:
  x86TofinoGenerator()
      : Target(GET_BOILERPLATE_PATH(X86_TOFINO_BOILERPLATE_FILE)) {}

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;
};

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse