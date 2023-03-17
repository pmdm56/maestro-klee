#pragma once

#include "node.h"

namespace BDD {

class Call : public Node {
private:
  call_t call;

public:
  Call(uint64_t _id, const klee::ConstraintManager &_constraints, call_t _call)
      : Node(_id, Node::NodeType::CALL, _constraints), call(_call) {}

  Call(uint64_t _id, const BDDNode_ptr &_next, const BDDNode_ptr &_prev,
       const klee::ConstraintManager &_constraints, call_t _call)
      : Node(_id, Node::NodeType::CALL, _next, _prev, _constraints),
        call(_call) {}

  call_t get_call() const { return call; }
  void set_call(call_t _call) { call = _call; }

  symbols_t get_node_generated_symbols() const override;

  virtual BDDNode_ptr clone(bool recursive = false) const override;
  virtual void recursive_update_ids(uint64_t &new_id) override;

  void visit(BDDVisitor &visitor) const override;
  std::string dump(bool one_liner = false) const;
};

} // namespace BDD