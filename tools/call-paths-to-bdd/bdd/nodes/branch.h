#pragma once

#include "node.h"

namespace BDD {
class Branch : public Node {
private:
  BDDNode_ptr on_false;
  klee::ref<klee::Expr> condition;

public:
  Branch(node_id_t _id, const klee::ConstraintManager &_constraints,
         klee::ref<klee::Expr> _condition)
      : Node(_id, Node::NodeType::BRANCH, _constraints), on_false(nullptr),
        condition(_condition) {}

  Branch(node_id_t _id, const BDDNode_ptr &_on_true, const BDDNode_ptr &_prev,
         const klee::ConstraintManager &_constraints,
         const BDDNode_ptr &_on_false, klee::ref<klee::Expr> _condition)
      : Node(_id, Node::NodeType::BRANCH, _on_true, _prev, _constraints),
        on_false(_on_false), condition(_condition) {}

  klee::ref<klee::Expr> get_condition() const { return condition; }
  void set_condition(const klee::ref<klee::Expr> &_condition) {
    condition = _condition;
  }

  const BDDNode_ptr &get_on_true() const { return next; }
  BDDNode_ptr get_on_true() { return next; }

  const BDDNode_ptr &get_on_false() const { return on_false; }
  BDDNode_ptr get_on_false() { return on_false; }

  virtual std::vector<node_id_t> get_terminating_node_ids() const override;

  void disconnect() {
    prev = nullptr;
    next = nullptr;
    on_false = nullptr;
  }

  void replace_on_true(const BDDNode_ptr &_on_true) { replace_next(_on_true); }
  void replace_on_false(const BDDNode_ptr &_on_false) { on_false = _on_false; }

  void add_on_true(const BDDNode_ptr &_on_true) { add_next(_on_true); }
  void add_on_false(const BDDNode_ptr &_on_false) { on_false = _on_false; }

  virtual BDDNode_ptr clone(bool recursive = false) const override;
  virtual void recursive_update_ids(node_id_t &new_id) override;

  void visit(BDDVisitor &visitor) const override;
  std::string dump(bool one_liner = false) const override;
  std::string dump_recursive(int lvl = 0) const override;
};

} // namespace BDD