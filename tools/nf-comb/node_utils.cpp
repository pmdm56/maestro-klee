#include "node_utils.h"

namespace nfcomb {

Node_ptr dup_node(const Node_ptr &node) {
  switch (node->get_type()) {
  case Node::NodeType::BRANCH: {
    auto original = static_cast<Branch *>(node.get());
    auto clone = std::make_shared<Branch>(
        original->get_id(), original->get_condition(), node->get_from_id());
    return clone;
  }
  case Node::NodeType::CALL: {
    auto original = static_cast<Call *>(node.get());
    auto clone = std::make_shared<Call>(
        original->get_id(), original->get_call(), node->get_from_id());
    return clone;
  }
  case Node::NodeType::RETURN_PROCESS: {
    auto original = static_cast<ReturnProcess *>(node.get());
    auto clone = std::make_shared<ReturnProcess>(
        original->get_id(), original->get_return_value(),
        original->get_return_operation(), node->get_from_id());
    return clone;
  }
  case Node::NodeType::RETURN_INIT: {
    auto original = static_cast<ReturnInit *>(node.get());
    auto clone = std::make_shared<ReturnInit>(
        original->get_id(), original->get_return_value(), node->get_from_id());
    return clone;
  }
  default:
    assert(false && "Cannot duplicate unkown type of node.");
    break;
  }
}

void insertAfter(BDD::Node_ptr &root, BDD::Node_ptr &after,
                 bool onTrue = true) {
  assert(after);
  assert(root);

  BDD::Node_ptr next_root;
  if (root->get_type() == BDD::Node::NodeType::BRANCH) {
    auto branch = static_cast<BDD::Branch *>(root.get());
    next_root = onTrue ? root->get_next() : branch->get_on_false();
  } else {
    next_root = root->get_next();
  }

  // root <-> after
  after->replace_prev(root);
  if (root->get_type() == BDD::Node::NodeType::BRANCH) {
    auto branch = static_cast<BDD::Branch *>(root.get());
    if (onTrue)
      branch->replace_on_true(after);
    else
      branch->replace_on_false(after);
  } else {
    root->replace_next(after);
  }

  // after <-> root.next
  if (next_root) {
    next_root->replace_prev(after);
    if (after->get_type() == BDD::Node::NodeType::BRANCH) {
      auto branch = static_cast<BDD::Branch *>(after.get());
      branch->replace_on_true(next_root);
      branch->replace_on_false(next_root->clone(true));
    } else {
      after->replace_next(next_root);
    }
  }
}

bool node_equals(BDD::Node_ptr n1, BDD::Node_ptr n2) {

  if (n1->get_type() != n2->get_type())
    return false;

  switch (n2->get_type()) {
  case BDD::Node::NodeType::RETURN_PROCESS: {
    auto rp1 = static_cast<BDD::ReturnProcess *>(n1.get());
    auto rp2 = static_cast<BDD::ReturnProcess *>(n2.get());

    if (rp1->get_return_operation() != rp2->get_return_operation())
      return false;

    if (rp1->get_return_operation() == BDD::ReturnProcess::Operation::FWD)
      return rp1->get_return_value() == rp2->get_return_value();

    return true;
  }
  case BDD::Node::NodeType::RETURN_INIT: {
    auto rp1 = static_cast<BDD::ReturnInit *>(n1.get());
    auto rp2 = static_cast<BDD::ReturnInit *>(n2.get());

    return rp1->get_return_value() == rp2->get_return_value();
  }
  case BDD::Node::NodeType::BRANCH: {

    auto b1 = static_cast<BDD::Branch *>(n1.get());
    auto b2 = static_cast<BDD::Branch *>(n2.get());

    // TODO: branch from packet and from data structures
    return kutil::solver_toolbox.are_exprs_always_equal(b1->get_condition(),
                                                        b2->get_condition());
  }
  case BDD::Node::NodeType::CALL: {
    auto c1 = static_cast<BDD::Call *>(n1.get());
    auto c2 = static_cast<BDD::Call *>(n2.get());

    if (c1->get_call().function_name == c2->get_call().function_name) {
      // global
      if (c1->get_call().function_name == "packet_borrow_next_chunk")
        return kutil::solver_toolbox.are_exprs_always_equal(
            c1->get_call().args["length"].expr,
            c2->get_call().args["length"].expr);

      // global
      if (c1->get_call().function_name == "packet_return_chunk") {
        auto out_c1 = c1->get_call().args["the_chunk"].in;
        auto out_c2 = c2->get_call().args["the_chunk"].in;
        return out_c1->getWidth() == out_c2->getWidth();
      }

          // global
      if (c1->get_call().function_name == "current_time") {
        return true;
      }

    }

    return kutil::solver_toolbox.are_calls_equal(c1->get_call(),
                                                 c2->get_call()) &&
           c1->get_from_id() == c2->get_from_id();
  }
  default:
    assert(false && "Cannot compare two nodes with unkown types.");
    break;
  }
}

// return process being added before another return process
void add_node(BDD::Node_ptr &root, BDD::Node_ptr &new_node,
              klee::ConstraintManager constraints) {

  if (node_equals(root, new_node)) {
    return;
  }

  switch (root->get_type()) {
  case BDD::Node::NodeType::BRANCH: {

    auto branch = static_cast<BDD::Branch *>(root.get());
    klee::ConstraintManager on_true_path_contrs;
    on_true_path_contrs.addConstraint(branch->get_condition());
    klee::ConstraintManager on_false_path_contrs;
    on_false_path_contrs.addConstraint(
        kutil::solver_toolbox.exprBuilder->Not(branch->get_condition()));
    auto new_node_clone = dup_node(new_node);

    // on true
    if (kutil::solver_toolbox.are_constraints_compatible(on_true_path_contrs,
                                                         constraints)) {

      auto next_root = branch->get_on_true();

      // prevent duplicate return process/init
      if (next_root)
        if (node_equals(next_root, new_node))
          return;

      if (!next_root ||
          (next_root->get_type() == BDD::Node::NodeType::RETURN_PROCESS) ||
          (next_root->get_type() == BDD::Node::NodeType::RETURN_INIT)) {
        insertAfter(root, new_node);
      } else {
        add_node(next_root, new_node, constraints);
      }
    }

    if (kutil::solver_toolbox.are_constraints_compatible(on_false_path_contrs,
                                                         constraints)) {
      auto next_root = branch->get_on_false();

      // prevent duplicate return process
      if (next_root)
        if (node_equals(next_root, new_node))
          return;

      if (!next_root ||
          (next_root->get_type() == BDD::Node::NodeType::RETURN_PROCESS) ||
          (next_root->get_type() == BDD::Node::NodeType::RETURN_INIT)) {
        insertAfter(root, new_node_clone, false);
      } else {
        add_node(next_root, new_node_clone, constraints);
      }
    }
    break;
  }
  case BDD::Node::NodeType::CALL: {

    auto call = static_cast<BDD::Call *>(root.get());
    auto next_root = call->get_next();

    // prevent duplicate return process / init
    if (next_root)
      if (node_equals(next_root, new_node))
        return;

    if (!next_root ||
        (next_root->get_type() == BDD::Node::NodeType::RETURN_PROCESS) ||
        (next_root->get_type() == BDD::Node::NodeType::RETURN_INIT)) {
      insertAfter(root, new_node);
    } else {
      add_node(next_root, new_node, constraints);
    }

    break;
  }
  case BDD::Node::NodeType::RETURN_PROCESS:
  case BDD::Node::NodeType::RETURN_INIT:
  default:
    assert(false);
    break;
  };
}

bool isPacketBorrow(BDD::Node_ptr node) {

  if (node->get_type() != BDD::Node::NodeType::CALL)
    return false;

  auto call = static_cast<BDD::Call *>(node.get());

  return call->get_call().function_name == "packet_borrow_next_chunk";
}

bool isPacketReturn(BDD::Node_ptr node) {

  if (node->get_type() != BDD::Node::NodeType::CALL)
    return false;

  auto call = static_cast<BDD::Call *>(node.get());

  return call->get_call().function_name == "packet_return_chunk";
}

} // namespace nfcomb