#pragma once

#include "../../../../log.h"
#include "../../../execution_plan.h"

#include "../../visitor.h"
#include "../target_code_generator.h"

namespace synapse {

class KleeExprToP4;
class KeysFromKleeExpr;

class TofinoGenerator : public TargetCodeGenerator {
  friend class KleeExprToP4;
  friend class KeysFromKleeExpr;

public:
  TofinoGenerator()
      : TargetCodeGenerator(GET_BOILERPLATE_PATH("boilerplate.p4")) {}

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;
  void visit(const targets::tofino::EthernetConsume *node) override;
};
} // namespace synapse
