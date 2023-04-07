#pragma once

#include "klee-util.h"
#include "modules/module.h"

#include <unordered_map>

namespace synapse {

class MemoryBank;
typedef std::shared_ptr<MemoryBank> MemoryBank_ptr;

struct reorder_data_t {
  bool valid;
  int candidate_node_id;
  klee::ref<klee::Expr> condition;

  reorder_data_t() : valid(false) {}

  reorder_data_t(const reorder_data_t &other)
      : valid(other.valid), candidate_node_id(other.candidate_node_id),
        condition(other.condition) {}

  reorder_data_t(int _candidate_node_id, klee::ref<klee::Expr> _condition)
      : valid(true), candidate_node_id(_candidate_node_id),
        condition(_condition) {}
};

enum PlacementDecision {
  TofinoTable,
};

typedef uint64_t obj_addr_t;

class MemoryBank {
private:
  std::vector<reorder_data_t> reorder_data;
  std::unordered_map<obj_addr_t, PlacementDecision> placement_decisions;

public:
  MemoryBank() {}
  MemoryBank(const MemoryBank &mb) : reorder_data(mb.reorder_data) {}

  reorder_data_t get_reorder_data(int node_id) const {
    for (const auto &data : reorder_data) {
      if (data.valid && data.candidate_node_id == node_id) {
        return data;
      }
    }

    return reorder_data_t();
  }

  void add_reorder_data(int node_candidate_id, klee::ref<klee::Expr> cond) {
    reorder_data.emplace_back(node_candidate_id, cond);
  }

  void save_placement_decision(klee::ref<klee::Expr> obj_addr,
                               PlacementDecision decision) {
    auto obj_addr_value = expr_addr_to_obj_addr(obj_addr);

    assert(placement_decisions.find(obj_addr_value) ==
           placement_decisions.end());

    placement_decisions[obj_addr_value] = decision;
  }

  bool has_placement_decision(klee::ref<klee::Expr> obj_addr) {
    auto obj_addr_value = expr_addr_to_obj_addr(obj_addr);
    auto found_it = placement_decisions.find(obj_addr_value);
    return found_it != placement_decisions.end();
  }

  bool check_placement_decision(klee::ref<klee::Expr> obj_addr,
                                PlacementDecision decision) const {
    auto obj_addr_value = expr_addr_to_obj_addr(obj_addr);
    auto found_it = placement_decisions.find(obj_addr_value);

    return found_it != placement_decisions.end() &&
           found_it->second == decision;
  }

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new MemoryBank(*this));
  }

  static MemoryBank_ptr build() { return MemoryBank_ptr(new MemoryBank()); }

  static obj_addr_t expr_addr_to_obj_addr(klee::ref<klee::Expr> obj_addr) {
    assert(!obj_addr.isNull());
    assert(kutil::is_constant(obj_addr));
    return kutil::solver_toolbox.value_from_expr(obj_addr);
  }
};

} // namespace synapse
