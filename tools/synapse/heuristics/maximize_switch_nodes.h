#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct MaximizeSwitchNodesComparator : public HeuristicConfiguration {
  Score get_score(const ExecutionPlan &e) const override {
    Score score(e);

    score.add(Score::Category::NumberOfIntAllocatorOps, Score::MAX);

    score.add(Score::Category::NumberOfMergedTables, Score::MAX);
    // score.add(Score::Category::NumberOfSimpleTables, Score::MAX);

    score.add(Score::Category::NumberOfSwitchNodes, Score::MAX);
    score.add(Score::Category::NumberOfSwitchLeaves, Score::MAX);
    score.add(Score::Category::HasNextStatefulOperationInSwitch, Score::MAX);
    score.add(Score::Category::ConsecutiveObjectOperationsInSwitch, Score::MAX);

    // Let's add this one to just speed up the process when we are generating
    // controller nodes. After all, we only get to this point if all the metrics
    // behind this one are the same, and by that point who cares.
    score.add(Score::Category::NumberOfControllerNodes, Score::MAX);

    return score;
  }

  bool terminate_on_first_solution() const override { return true; }
};

using MaximizeSwitchNodes = Heuristic<MaximizeSwitchNodesComparator>;
} // namespace synapse
