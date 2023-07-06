#pragma once

#include "node.h"

namespace BDD {

class Call : public Node {
private:
  call_t call;

public:

  Call(uint64_t _id, call_t _call)
      : Node(_id, Node::NodeType::CALL), call(_call) {}
  
  Call(uint64_t _id, call_t _call, int bdd_id, std::string bdd_name)
      : Node(_id, Node::NodeType::CALL, bdd_id, bdd_name), call(_call) {}

  Call(node_id_t _id, const klee::ConstraintManager &_constraints, call_t _call)
      : Node(_id, Node::NodeType::CALL, _constraints), call(_call) {}

  Call(node_id_t _id, const Node_ptr &_next, const Node_ptr &_prev,
       const klee::ConstraintManager &_constraints, call_t _call)
      : Node(_id, Node::NodeType::CALL, _next, _prev, _constraints),
        call(_call) {}

  call_t get_call() const { return call; }
  void set_call(call_t _call) { call = _call; }

  symbols_t get_local_generated_symbols() const override;

  virtual Node_ptr clone(bool recursive = false) const override;
  virtual void recursive_update_ids(node_id_t &new_id) override;

  void visit(BDDVisitor &visitor) const override;
  std::string dump(bool one_liner = false) const;
};

#define BDD_CAST_CALL(NODE_PTR) (static_cast<BDD::Call *>((NODE_PTR).get()))

} // namespace BDD