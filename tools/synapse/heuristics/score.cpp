#include "score.h"
#include "../execution_plan/execution_plan.h"
#include "../execution_plan/modules/modules.h"

namespace synapse {

int Score::get_nr_nodes() const { return execution_plan.get_nodes(); }

int Score::get_nr_merged_tables() const {
  if (!execution_plan.get_root()) {
    return 0;
  }

  auto num_merged_tables = 0;
  auto nodes = std::vector<ExecutionPlanNode_ptr>{execution_plan.get_root()};

  while (nodes.size()) {
    auto node = nodes[0];
    nodes.erase(nodes.begin());

    auto module = node->get_module();
    if (module->get_type() == Module::ModuleType::BMv2_TableLookup) {
      auto tableLookup =
          static_cast<targets::bmv2::TableLookup *>(module.get());

      auto merged = tableLookup->get_keys().size();

      if (merged > 1) {
        num_merged_tables += merged;
      }
    }

    for (auto branch : node->get_next()) {
      nodes.push_back(branch);
    }
  }

  return num_merged_tables;
}

int Score::get_depth() const { return execution_plan.get_depth(); }

int Score::get_nr_switch_nodes() const {
  auto switch_nodes = 0;

  const auto &nodes_per_target = execution_plan.get_nodes_per_target();

  auto bmv2_nodes_it = nodes_per_target.find(TargetType::BMv2);
  auto tofino_nodes_it = nodes_per_target.find(TargetType::Tofino);

  if (bmv2_nodes_it != nodes_per_target.end()) {
    switch_nodes += bmv2_nodes_it->second;
  }

  if (tofino_nodes_it != nodes_per_target.end()) {
    switch_nodes += tofino_nodes_it->second;
  }

  return switch_nodes;
}

int Score::get_nr_controller_nodes() const {
  auto controller_nodes = 0;
  const auto &nodes_per_target = execution_plan.get_nodes_per_target();

  auto bmv2_controller_nodes_it = nodes_per_target.find(TargetType::x86_BMv2);
  auto tofino_controller_nodes_it =
      nodes_per_target.find(TargetType::x86_Tofino);

  if (bmv2_controller_nodes_it != nodes_per_target.end()) {
    controller_nodes += bmv2_controller_nodes_it->second;
  }

  if (tofino_controller_nodes_it != nodes_per_target.end()) {
    controller_nodes += tofino_controller_nodes_it->second;
  }

  return controller_nodes;
}

int Score::get_nr_reordered_nodes() const {
  return execution_plan.get_reordered_nodes();
}

int Score::get_nr_exact_match_nodes() const {
  auto targets = std::vector<DummyModule>{target_ep};
  auto nodes = std::vector<ExecutionPlanNode_ptr>{execution_plan.get_root()};
  auto matches = 0;

  while (nodes.size() && targets.size()) {
    auto node = nodes[0];
    auto target = targets[0];

    nodes.erase(nodes.begin());
    targets.erase(targets.begin());

    assert(node);
    auto module = node->get_module();

    if (module->get_type() != target.type) {
      break;
    }

    matches++;

    auto next_node = node->get_next();
    auto next_target = target.next;

    nodes.insert(nodes.end(), next_node.begin(), next_node.end());
    targets.insert(targets.end(), next_target.begin(), next_target.end());
  }

  return matches;
}

int Score::get_nr_switch_leaves() const {
  int switch_leaves = 0;
  auto leaves = execution_plan.get_leaves();
  auto switch_types =
      std::vector<TargetType>{TargetType::BMv2, TargetType::Tofino};

  for (auto leaf : leaves) {
    if (!leaf.current_platform.first) {
      continue;
    }

    auto found_it = std::find(switch_types.begin(), switch_types.end(),
                              leaf.current_platform.second);

    if (found_it != switch_types.end()) {
      switch_leaves++;
    }
  }

  return switch_leaves;
}

} // namespace synapse
