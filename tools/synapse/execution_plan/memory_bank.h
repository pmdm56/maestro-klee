#pragma once

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

class MemoryBank {
private:
  std::vector<reorder_data_t> reorder_data;

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

  virtual MemoryBank_ptr clone() const {
    return MemoryBank_ptr(new MemoryBank(*this));
  }

  static MemoryBank_ptr build() { return MemoryBank_ptr(new MemoryBank()); }
};

} // namespace synapse
