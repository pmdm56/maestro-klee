#pragma once

#include "call-paths-to-bdd.h"

#include "../log.h"
#include "execution_plan_node.h"
#include "visitors/visitor.h"

#include <unordered_set>

namespace synapse {

class ExecutionPlan {
public:
  struct leaf_t {
    ExecutionPlanNode_ptr leaf;
    const BDD::Node *next;

    leaf_t(const BDD::Node *_next) : next(_next) {}

    leaf_t(ExecutionPlanNode_ptr _leaf, const BDD::Node *_next)
        : leaf(_leaf), next(_next) {}

    leaf_t(const leaf_t &_leaf) : leaf(_leaf.leaf), next(_leaf.next) {}
  };

private:
  ExecutionPlanNode_ptr root;
  std::vector<leaf_t> leaves;

  std::shared_ptr<BDD::BDD> bdd;

  // Implementation details
private:
  std::unordered_set<uint64_t> processed_bdd_nodes;

  // When reordering nodes, it can happen that a function call
  // that is called on different call paths is collapsed into
  // a single one. We have to merge both their out expressions
  // into a single one.
  std::map<std::string, std::string> colapsed_symbols;

  // Metadata
private:
  unsigned depth;
  unsigned nodes;
  unsigned reordered_nodes;

  unsigned id;

  static int counter;

public:
  ExecutionPlan(const BDD::BDD *_bdd)
      : bdd(_bdd->clone()), depth(0), nodes(0), reordered_nodes(0),
        id(counter++) {
    assert(bdd->get_process());
    leaf_t leaf(bdd->get_process());
    leaves.push_back(leaf);
  }

  ExecutionPlan(const ExecutionPlan &ep)
      : root(ep.root), leaves(ep.leaves), bdd(ep.bdd),
        processed_bdd_nodes(ep.processed_bdd_nodes), depth(ep.depth),
        nodes(ep.nodes), reordered_nodes(ep.reordered_nodes), id(ep.id) {}

  ExecutionPlan(const ExecutionPlan &ep, const leaf_t &leaf,
                const BDD::BDD *_bdd)
      : ExecutionPlan(ep.clone()) {
    id = counter++;
    add(leaf);
  }

  ExecutionPlan(const ExecutionPlan &ep, const std::vector<leaf_t> &_leaves,
                const BDD::BDD *_bdd)
      : ExecutionPlan(ep.clone()) {
    assert(root);
    id = counter++;
    add(_leaves);
  }

private:
  void update_leaves(leaf_t leaf) {
    leaves.erase(leaves.begin());
    if (leaf.next) {
      leaves.insert(leaves.begin(), leaf);
    }
  }

  void update_leaves(std::vector<leaf_t> _leaves) {
    assert(leaves.size());
    leaves.erase(leaves.begin());
    for (auto leaf : _leaves) {
      leaves.insert(leaves.begin(), leaf);
    }
  }

  ExecutionPlanNode_ptr clone_nodes(ExecutionPlan &ep,
                                    const ExecutionPlanNode *node) const {
    auto copy = ExecutionPlanNode::build(node);
    auto old_next = node->get_next();
    Branches new_next;

    for (auto branch : old_next) {
      auto branch_copy = clone_nodes(ep, branch.get());
      new_next.push_back(branch_copy);
      branch_copy->set_prev(copy);
    }

    if (new_next.size()) {
      copy->set_next(new_next);
      return copy;
    }

    for (auto &leaf : ep.leaves) {
      if (leaf.leaf->get_id() == node->get_id()) {
        leaf.leaf = copy;
      }
    }

    return copy;
  }

  void update_processed_nodes() {
    assert(leaves.size());
    if (!leaves[0].next)
      return;

    auto processed_node = leaves[0].next;

    auto processed_node_id = processed_node->get_id();
    auto search = processed_bdd_nodes.find(processed_node_id);
    assert(search == processed_bdd_nodes.end());
    processed_bdd_nodes.insert(processed_node_id);
  }

  void add(const leaf_t &leaf) {
    update_processed_nodes();

    if (!root) {
      assert(leaves.size() == 1);
      assert(!leaves[0].leaf);
      root = leaf.leaf;
    } else {
      assert(root);
      assert(leaves.size());
      leaves[0].leaf->set_next(Branches{ leaf.leaf });
      leaf.leaf->set_prev(leaves[0].leaf);
    }

    depth++;
    nodes++;

    assert(leaves.size());
    update_leaves(leaf);
  }

  // Order matters!
  // The active leaf will correspond to the first branch in the branches
  void add(const std::vector<leaf_t> &_leaves) {
    assert(root);
    assert(leaves.size());

    update_processed_nodes();

    Branches branches;
    for (auto &leaf : _leaves) {
      branches.push_back(leaf.leaf);
      assert(!leaf.leaf->get_prev());
      leaf.leaf->set_prev(leaves[0].leaf);
      nodes++;
    }

    leaves[0].leaf->set_next(branches);

    depth++;
    update_leaves(_leaves);
  }

  void replace_node_in_bdd(BDD::Node *target);

public:
  unsigned get_depth() const { return depth; }
  unsigned get_nodes() const { return nodes; }
  unsigned get_id() const { return id; }
  unsigned get_reordered_nodes() const { return reordered_nodes; }

  const ExecutionPlanNode_ptr &get_root() const { return root; }

  const BDD::Node *get_next_node() const {
    const BDD::Node *next = nullptr;

    if (leaves.size()) {
      next = leaves[0].next;
    }

    return next;
  }

  ExecutionPlanNode_ptr get_active_leaf() const {
    ExecutionPlanNode_ptr leaf;

    if (leaves.size()) {
      leaf = leaves[0].leaf;
    }

    return leaf;
  }

  ExecutionPlan clone_and_replace_active_leaf_node(BDD::Node *node) const {
    auto new_ep = clone(true);

    assert(leaves.size());
    assert(node);
    assert(node->get_type() != BDD::Node::NodeType::BRANCH);

    auto old_active_leaf = leaves[0].leaf;
    auto module = old_active_leaf->get_module();
    auto cloned = module->clone();
    cloned->replace_node(node);

    new_ep.leaves[0].leaf->replace_module(cloned);
    new_ep.leaves[0].next = node->get_next();
    new_ep.reordered_nodes++;

    new_ep.replace_node_in_bdd(node);

    return new_ep;
  }

  const std::vector<leaf_t> &get_leaves() const { return leaves; }

  const BDD::BDD *get_bdd() const {
    assert(bdd);
    return bdd.get();
  }

  const std::unordered_set<uint64_t> &get_processed_bdd_nodes() const {
    return processed_bdd_nodes;
  }

  void visit(ExecutionPlanVisitor &visitor) const { visitor.visit(*this); }

  ExecutionPlan clone(bool deep = false) const {
    ExecutionPlan copy = *this;

    if (deep) {
      copy.bdd = copy.bdd->clone();
    }

    if (root) {
      copy.root = clone_nodes(copy, root.get());
    } else {
      for (auto leaf : leaves) {
        assert(!leaf.leaf);
      }
    }

    return copy;
  }
};
} // namespace synapse