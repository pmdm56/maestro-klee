#pragma once

#include "call-paths-to-bdd.h"

#include "../log.h"
#include "../modules/module.h"
#include "visitors/visitor.h"

namespace synapse {

class ExecutionPlanNode;
typedef std::shared_ptr<ExecutionPlanNode> ExecutionPlanNode_ptr;
typedef std::vector<ExecutionPlanNode_ptr> Branches;

class ExecutionPlanNode {
  friend class ExecutionPlan;

private:
  Module_ptr module;
  Branches next;
  ExecutionPlanNode_ptr prev;
  const BDD::Node *node;
  int id;

  static int counter;

private:
  ExecutionPlanNode(Module_ptr _module)
      : module(_module), node(_module->get_node()), id(counter++) {}

  ExecutionPlanNode(const ExecutionPlanNode *ep_node)
      : module(ep_node->module), node(ep_node->node), id(counter++) {}

public:
  void set_next(Branches _next) {
    assert(!next.size());
    next = _next;
  }

  void set_next(ExecutionPlanNode_ptr _next) {
    assert(!next.size());
    next.push_back(_next);
  }

  void set_prev(ExecutionPlanNode_ptr _prev) {
    assert(!prev);
    prev = _prev;
  }

  const Module_ptr &get_module() const { return module; }
  void replace_module(Module_ptr _module) { module = _module; }

  const Branches &get_next() const { return next; }
  ExecutionPlanNode_ptr get_prev() const { return prev; }
  int get_id() const { return id; }

  void replace_next(ExecutionPlanNode_ptr before, ExecutionPlanNode_ptr after) {
    for (auto &branch : next) {
      if (branch->get_id() == before->get_id()) {
        branch = after;
        return;
      }
    }

    assert(false && "Before ExecutionPlanNode not found");
  }

  void replace_prev(ExecutionPlanNode_ptr _prev) { prev = _prev; }

  void visit(ExecutionPlanVisitor &visitor) const { visitor.visit(this); }

  static ExecutionPlanNode_ptr build(Module_ptr _module) {
    ExecutionPlanNode *epn = new ExecutionPlanNode(_module);
    return std::shared_ptr<ExecutionPlanNode>(epn);
  }

  static ExecutionPlanNode_ptr build(const ExecutionPlanNode *ep_node) {
    ExecutionPlanNode *epn = new ExecutionPlanNode(ep_node);
    return std::shared_ptr<ExecutionPlanNode>(epn);
  }
};
} // namespace synapse