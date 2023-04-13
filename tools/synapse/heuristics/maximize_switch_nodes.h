#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct MaximizeSwitchNodesComparator : public HeuristicConfiguration {
  Score get_score(const ExecutionPlan &e) const override {
    Score score(e);

    score.add(Score::Category::NumberOfMergedTables, Score::MAX);
    score.add(Score::Category::NumberOfSwitchNodes, Score::MAX);
    score.add(Score::Category::NumberOfSwitchLeaves, Score::MAX);
    score.add(Score::Category::HasNextStatefulOperation, Score::MAX);
    score.add(Score::Category::ConsecutiveObjectOperations, Score::MAX);
    // score.add(Score::Category::NumberOfNodes, Score::MIN);

    return score;
  }

  bool terminate_on_first_solution() const override { return true; }
};

using MaximizeSwitchNodes = Heuristic<MaximizeSwitchNodesComparator>;
} // namespace synapse
