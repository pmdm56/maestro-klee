#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct MaximizeSwitchNodesComparator : public HeuristicConfiguration {
  Score get_score(const ExecutionPlan &ep) const override {
    Score score(
        ep,
        {
            // {Score::Category::NumberOfIntAllocatorOps, Score::MAX},
            // {Score::Category::NumberOfSimpleTables, Score::MAX},
            {Score::Category::NumberOfCounters, Score::MAX},
            {Score::Category::NumberOfSwitchNodes, Score::MAX},
            {Score::Category::NumberOfSwitchLeaves, Score::MAX},
            // {Score::Category::HasNextStatefulOperationInSwitch, Score::MAX},
            {Score::Category::ConsecutiveObjectOperationsInSwitch, Score::MAX},

            // Let's add this one to just speed up the process when we are
            // generating controller nodes. After all, we only get to this point
            // if all the metrics behind this one are the same, and by that
            // point who cares.
            {Score::Category::ProcessedBDDPercentage, Score::MAX},
        });

    return score;
  }

  bool terminate_on_first_solution() const override { return true; }
};

using MaximizeSwitchNodes = Heuristic<MaximizeSwitchNodesComparator>;
} // namespace synapse
