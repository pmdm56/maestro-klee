#pragma once

#include "execution_plan/execution_plan.h"
#include "execution_plan/visitors/graphviz/graphviz.h"
#include "heuristics/heuristic.h"
#include "log.h"
#include "search_space.h"

namespace synapse {

class SearchEngine {
private:
  std::vector<Target_ptr> targets;
  BDD::BDD bdd;

  // Maximum number of reordered nodes on the BDD
  // -1 => unlimited
  int max_reordered;

public:
  SearchEngine(BDD::BDD _bdd, int _max_reordered)
      : bdd(_bdd), max_reordered(_max_reordered) {}

  SearchEngine(const SearchEngine &se)
      : SearchEngine(se.bdd, se.max_reordered) {
    targets = se.targets;
  }

public:
  void add_target(TargetType target) {
    switch (target) {
    case TargetType::x86_BMv2:
      targets.push_back(targets::x86_bmv2::x86BMv2Target::build());
      break;
    case TargetType::x86_Tofino:
      targets.push_back(targets::x86_tofino::x86TofinoTarget::build());
      break;
    case TargetType::Tofino:
      targets.push_back(targets::tofino::TofinoTarget::build());
      break;
    case TargetType::Netronome:
      targets.push_back(targets::netronome::NetronomeTarget::build());
      break;
    case TargetType::FPGA:
      targets.push_back(targets::fpga::FPGATarget::build());
      break;
    case TargetType::BMv2:
      targets.push_back(targets::bmv2::BMv2Target::build());
      break;
    }
  }

  template <class T> ExecutionPlan search(Heuristic<T> h) {
    auto first_execution_plan = ExecutionPlan(bdd);

    for (auto target : targets) {
      first_execution_plan.add_target(target->type, target->memory_bank);
    }

    SearchSpace search_space(h.get_cfg(), first_execution_plan);
    h.add(std::vector<ExecutionPlan>{first_execution_plan});

    while (!h.finished()) {
      auto available = h.size();
      auto next_ep = h.pop();
      auto next_node = next_ep.get_next_node();
      assert(next_node);

      struct report_t {
        std::vector<std::string> target_name;
        std::vector<std::string> name;
        std::vector<unsigned> generated_contexts;
        std::vector<std::vector<unsigned>> generated_exec_plans_ids;
      };

      report_t report;

      for (auto target : targets) {
        for (auto module : target->modules) {
          auto result = module->process_node(next_ep, next_node, max_reordered);

          if (result.next_eps.size()) {
            report.target_name.push_back(module->get_target_name());
            report.name.push_back(module->get_name());
            report.generated_contexts.push_back(result.next_eps.size());
            report.generated_exec_plans_ids.emplace_back();

            for (const auto &ep : result.next_eps) {
              report.generated_exec_plans_ids.back().push_back(ep.get_id());
            }

            h.add(result.next_eps);
            search_space.add_leaves(next_ep, result.module, result.next_eps);
          }
        }
      }

      if (report.target_name.size()) {
        search_space.submit_leaves();

        Log::dbg() << "\n";
        Log::dbg()
            << "=======================================================\n";
        Log::dbg() << "Available      " << available << "\n";
        Log::dbg() << "Execution Plan " << next_ep.get_id() << "\n";
        Log::dbg() << "BDD progress   " << std::fixed << std::setprecision(2)
                   << 100 * next_ep.get_percentage_of_processed_bdd_nodes()
                   << " %"
                   << "\n";

        auto leaf = next_ep.get_active_leaf();
        if (leaf && leaf->get_module()) {
          Log::dbg() << "Leaf           " << leaf->get_module()->get_name()
                     << "\n";
        }

        Log::dbg() << "Node           " << next_node->dump(true) << "\n";

        auto platform = next_ep.get_current_platform();
        Log::dbg() << "Current target " << Module::target_to_string(platform)
                   << "\n";

        for (unsigned i = 0; i < report.target_name.size(); i++) {
          std::stringstream exec_plans_ids;
          exec_plans_ids << "[";
          for (auto j = 0u; j < report.generated_exec_plans_ids[i].size();
               j++) {
            if (j != 0) {
              exec_plans_ids << ",";
            }
            exec_plans_ids << report.generated_exec_plans_ids[i][j];
          }
          exec_plans_ids << "]";

          Log::dbg() << "MATCH          " << report.target_name[i]
                     << "::" << report.name[i] << " -> " << exec_plans_ids.str()
                     << " (" << report.generated_contexts[i] << ") exec plans "
                     << "\n";
        }

        Log::dbg()
            << "=======================================================\n";
      } else {
        Log::dbg() << "\n";
        Log::dbg()
            << "=======================================================\n";
        Log::dbg() << "Available      " << available << "\n";
        Log::dbg() << "Node           " << next_node->dump(true) << "\n";

        auto platform = next_ep.get_current_platform();
        Log::dbg() << "Current target " << Module::target_to_string(platform)
                   << "\n";

        Log::wrn() << "No module can handle this BDD node"
                      " in the current context.\n";
        Log::wrn() << "Deleting solution from search space.\n";

        Log::dbg()
            << "=======================================================\n";
      }

      // Graphviz::visualize(next_ep);
    }

    Log::log() << "Solutions:      " << h.get_all().size() << "\n";
    Log::log() << "Winner:         " << h.get_score(h.get()) << "\n";

    // Graphviz::visualize(h.get());
    // Graphviz::visualize(h.get_all().back());

    // for (auto &ep : h.get_all()) {
    //   std::cerr << "score: " << h.get_score(ep) << "\n";
    //   Graphviz::visualize(ep);
    // }

    // Graphviz::visualize(h.get(), search_space);

    return h.get();
  }
};

} // namespace synapse
