#pragma once

#include "call-paths-to-bdd.h"
#include "target.h"

#include <unordered_set>
#include <unordered_map>

namespace synapse {

class Module;
typedef std::shared_ptr<Module> Module_ptr;

class MemoryBank;
typedef std::shared_ptr<MemoryBank> MemoryBank_ptr;

class ExecutionPlanNode;
typedef std::shared_ptr<ExecutionPlanNode> ExecutionPlanNode_ptr;

class ExecutionPlanVisitor;

class ExecutionPlan {
  friend class ExecutionPlanNode;

public:
  struct leaf_t {
    ExecutionPlanNode_ptr leaf;
    BDD::BDDNode_ptr next;
    std::pair<bool, TargetType> current_platform;

    leaf_t(BDD::BDDNode_ptr _next);
    leaf_t(Module_ptr _module, BDD::BDDNode_ptr _next);
    leaf_t(const leaf_t &_leaf);

    leaf_t &operator=(const leaf_t &) = default;
  };

private:
  ExecutionPlanNode_ptr root;
  std::vector<leaf_t> leaves;
  BDD::BDD bdd;

  MemoryBank_ptr shared_memory_bank;
  std::unordered_map<TargetType, MemoryBank_ptr> memory_banks;
  std::unordered_set<uint64_t> processed_bdd_nodes;

  unsigned depth;
  unsigned nodes;
  std::map<TargetType, unsigned> nodes_per_target;
  unsigned reordered_nodes;
  unsigned id;

  static int counter;

public:
  ExecutionPlan(const BDD::BDD &_bdd);
  ExecutionPlan(const ExecutionPlan &ep);
  ExecutionPlan(const ExecutionPlan &ep, ExecutionPlanNode_ptr _root);

private:
  void update_leaves(std::vector<leaf_t> _leaves, bool is_terminal);
  void update_processed_nodes();

  ExecutionPlanNode_ptr clone_nodes(ExecutionPlan &ep,
                                    const ExecutionPlanNode *node) const;

public:
  unsigned get_depth() const;
  unsigned get_nodes() const;

  const std::map<TargetType, unsigned> &get_nodes_per_target() const;

  unsigned get_id() const;
  const std::vector<leaf_t> &get_leaves() const;
  const BDD::BDD &get_bdd() const;
  BDD::BDD &get_bdd();
  unsigned get_reordered_nodes() const;
  void inc_reordered_nodes();
  const ExecutionPlanNode_ptr &get_root() const;

  void add_memory_bank(TargetType type, MemoryBank_ptr mb);

  MemoryBank_ptr get_memory_bank() const;
  MemoryBank_ptr get_memory_bank(TargetType type) const;
  
  const std::unordered_set<uint64_t> &get_processed_bdd_nodes() const;

  BDD::BDDNode_ptr get_next_node() const;
  ExecutionPlanNode_ptr get_active_leaf() const;
  std::pair<bool, TargetType> get_current_platform() const;

  ExecutionPlan replace_leaf(Module_ptr new_module,
                             const BDD::BDDNode_ptr &next,
                             bool process_bdd_node = true) const;

  ExecutionPlan ignore_leaf(const BDD::BDDNode_ptr &next,
                            TargetType next_target,
                            bool process_bdd_node = true) const;

  ExecutionPlan add_leaves(Module_ptr new_module, const BDD::BDDNode_ptr &next,
                           bool is_terminal = false,
                           bool process_bdd_node = true) const;

  // Order matters!
  // The active leaf will correspond to the first branch in the branches
  ExecutionPlan add_leaves(std::vector<leaf_t> _leaves,
                           bool is_terminal = false,
                           bool process_bdd_node = true) const;

  void replace_active_leaf_node(BDD::BDDNode_ptr next,
                                bool process_bdd_node = true);

  float get_percentage_of_processed_bdd_nodes() const;
  void remove_from_processed_bdd_nodes(uint64_t id);
  void add_processed_bdd_node(uint64_t id);

  void visit(ExecutionPlanVisitor &visitor) const;

  ExecutionPlan clone(BDD::BDD new_bdd) const;
  ExecutionPlan clone(bool deep = false) const;
};

bool operator==(const ExecutionPlan &lhs, const ExecutionPlan &rhs);

} // namespace synapse