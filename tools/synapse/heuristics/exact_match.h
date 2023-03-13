#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct ExactMatchComparator : public HeuristicConfiguration {
  DummyModule target;

  ExactMatchComparator()
      : target({
            Module::ModuleType::Tofino_SendToController,
            {
                {
                    Module::ModuleType::x86_Tofino_PacketParseCPU,
                    {},
                },
            },
        }) {}

  Score get_score(const ExecutionPlan &e) const override {
    Score score(e);

    score.add(Score::Category::ExactMatchNodes);
    score.add_target_execution_plan(target);

    return score;
  }

  bool terminate_on_first_solution() const override { return true; }
};

using ExactMatch = Heuristic<ExactMatchComparator>;
} // namespace synapse
