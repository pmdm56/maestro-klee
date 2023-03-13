#include "execution_plan_node.h"
#include "../log.h"
#include "modules/module.h"
#include "visitors/visitor.h"

namespace synapse {

ExecutionPlanNode::ExecutionPlanNode(Module_ptr _module)
    : module(_module), id(counter++) {}

ExecutionPlanNode::ExecutionPlanNode(const ExecutionPlanNode *ep_node)
    : module(ep_node->module), id(counter++) {}

void ExecutionPlanNode::set_next(Branches _next) { next = _next; }
void ExecutionPlanNode::set_next(ExecutionPlanNode_ptr _next) {
  next.push_back(_next);
}
void ExecutionPlanNode::set_prev(ExecutionPlanNode_ptr _prev) { prev = _prev; }

const Module_ptr &ExecutionPlanNode::get_module() const { return module; }
void ExecutionPlanNode::replace_module(Module_ptr _module) { module = _module; }

const Branches &ExecutionPlanNode::get_next() const { return next; }
ExecutionPlanNode_ptr ExecutionPlanNode::get_prev() const { return prev; }

int ExecutionPlanNode::get_id() const { return id; }
void ExecutionPlanNode::set_id(int _id) { id = _id; }

void ExecutionPlanNode::replace_next(ExecutionPlanNode_ptr before,
                                     ExecutionPlanNode_ptr after) {
  for (auto &branch : next) {
    if (branch->get_id() == before->get_id()) {
      branch = after;
      return;
    }
  }

  assert(false && "Before ExecutionPlanNode not found");
}

void ExecutionPlanNode::replace_prev(ExecutionPlanNode_ptr _prev) {
  prev = _prev;
}

void ExecutionPlanNode::replace_node(BDD::BDDNode_ptr node) {
  module->replace_node(node);
}

void ExecutionPlanNode::visit(ExecutionPlanVisitor &visitor) const {
  visitor.visit(this);
}

ExecutionPlanNode_ptr ExecutionPlanNode::build(Module_ptr _module) {
  ExecutionPlanNode *epn = new ExecutionPlanNode(_module);
  return std::shared_ptr<ExecutionPlanNode>(epn);
}

ExecutionPlanNode_ptr
ExecutionPlanNode::build(const ExecutionPlanNode *ep_node) {
  ExecutionPlanNode *epn = new ExecutionPlanNode(ep_node);
  return std::shared_ptr<ExecutionPlanNode>(epn);
}

} // namespace synapse