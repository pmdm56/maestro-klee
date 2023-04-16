#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct DFSComparator : public HeuristicConfiguration {
  virtual Score get_score(const ExecutionPlan &e) const override {
    Score score(e);
    score.add(Score::Category::Depth, Score::MAX);
    return score;
  }

  bool terminate_on_first_solution() const override { return true; }
};

using DFS = Heuristic<DFSComparator>;
} // namespace synapse
