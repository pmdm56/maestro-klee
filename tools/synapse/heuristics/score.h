#pragma once

#include "../execution_plan/execution_plan.h"
#include "../execution_plan/execution_plan_node.h"
#include "../execution_plan/modules/modules.h"
#include "../log.h"

#include <iostream>
#include <map>
#include <vector>

namespace synapse {

struct DummyModule {
  Module::ModuleType type;
  std::vector<DummyModule> next;
};

class Score {
public:
  enum Category {
    NumberOfReorderedNodes,
    NumberOfSwitchNodes,
    NumberOfSwitchLeaves,
    NumberOfNodes,
    NumberOfControllerNodes,
    NumberOfMergedTables,
    NumberOfSimpleTables,
    Depth,
    ExactMatchNodes,
    ConsecutiveObjectOperationsInSwitch,
    HasNextStatefulOperationInSwitch,
  };

  enum Objective { MIN, MAX };

private:
  typedef int (Score::*ComputerPtr)() const;

  const ExecutionPlan &execution_plan;
  std::map<Category, ComputerPtr> computers;

  // The order of the elements in this vector matters.
  // It defines a lexicographic order.
  std::vector<std::pair<Category, Objective>> categories;

  int get_nr_nodes() const;
  int get_nr_merged_tables() const;
  int get_nr_simple_tables() const;
  int get_depth() const;
  int get_nr_switch_nodes() const;
  int get_nr_controller_nodes() const;
  int get_nr_reordered_nodes() const;
  int get_nr_exact_match_nodes() const;
  int get_nr_switch_leaves() const;
  int next_op_same_obj_in_switch() const;
  int next_op_is_stateful_in_switch() const;

  DummyModule target_ep;

public:
  Score(const ExecutionPlan &_execution_plan)
      : execution_plan(_execution_plan) {
    computers = {
        {NumberOfReorderedNodes, &Score::get_nr_reordered_nodes},
        {NumberOfNodes, &Score::get_nr_nodes},
        {NumberOfMergedTables, &Score::get_nr_merged_tables},
        {NumberOfSwitchNodes, &Score::get_nr_switch_nodes},
        {NumberOfSimpleTables, &Score::get_nr_simple_tables},
        {NumberOfSwitchLeaves, &Score::get_nr_switch_leaves},
        {NumberOfControllerNodes, &Score::get_nr_controller_nodes},
        {Depth, &Score::get_depth},
        {ExactMatchNodes, &Score::get_nr_exact_match_nodes},
        {ConsecutiveObjectOperationsInSwitch,
         &Score::next_op_same_obj_in_switch},
        {HasNextStatefulOperationInSwitch,
         &Score::next_op_is_stateful_in_switch},
    };
  }

  Score(const Score &score)
      : execution_plan(score.execution_plan), computers(score.computers),
        categories(score.categories) {}

  void add_target_execution_plan(const DummyModule &_target_ep) {
    target_ep = _target_ep;
  }

  void add(Category category, Objective objective) {
    auto found_it =
        std::find_if(categories.begin(), categories.end(),
                     [&](const std::pair<Category, Objective> &saved) {
                       return saved.first == category;
                     });

    assert(found_it == categories.end() && "Category already inserted");

    categories.emplace_back(category, objective);
  }

  int get(Category category) const {
    auto found_it = computers.find(category);

    if (found_it == computers.end()) {
      Log::err() << "\nScore error: " << category
                 << " not found in lookup table.\n";
      exit(1);
    }

    auto computer = found_it->second;
    return (this->*computer)();
  }

  inline bool operator<(const Score &other) {
    for (auto category_objective : categories) {
      auto category = category_objective.first;
      auto objective = category_objective.second;

      auto this_score = get(category);
      auto other_score = other.get(category);

      if (objective == Objective::MIN) {
        this_score *= -1;
        other_score *= -1;
      }

      if (this_score > other_score) {
        return false;
      }

      if (this_score < other_score) {
        return true;
      }
    }

    return false;
  }

  inline bool operator==(const Score &other) {
    for (auto category_objective : categories) {
      auto category = category_objective.first;
      auto objective = category_objective.second;

      auto this_score = get(category);
      auto other_score = other.get(category);

      if (objective == Objective::MIN) {
        this_score *= -1;
        other_score *= -1;
      }

      if (this_score != other_score) {
        return false;
      }
    }

    return true;
  }

  inline bool operator>(const Score &other) {
    return !((*this) < other) && !((*this) == other);
  }

  inline bool operator<=(const Score &other) { return !((*this) > other); }
  inline bool operator>=(const Score &other) { return !((*this) < other); }
  inline bool operator!=(const Score &other) { return !((*this) == other); }

  friend std::ostream &operator<<(std::ostream &os, const Score &dt);
};

inline std::ostream &operator<<(std::ostream &os, const Score &score) {
  os << "<";

  bool first = true;
  for (auto category_objective : score.categories) {
    auto category = category_objective.first;
    auto objective = category_objective.second;

    auto value = score.get(category);

    if (objective == Score::Objective::MIN) {
      value *= -1;
    }

    if (!first) {
      os << ",";
    }

    os << value;

    first &= false;
  }

  os << ">";
  return os;
}
} // namespace synapse