#pragma once

#include "bdd/nodes/nodes.h"

using namespace BDD;

namespace nfcomb {
Node_ptr dup_node(const Node_ptr &node);
void insertAfter(BDD::Node_ptr& root, BDD::Node_ptr& after, bool onTrue);
void add_node(BDD::Node_ptr &root, BDD::Node_ptr &new_node, klee::ConstraintManager constraints);
bool node_equals(BDD::Node_ptr n1, BDD::Node_ptr n2);
bool isPacketReturn(BDD::Node_ptr node);
bool isPacketBorrow(BDD::Node_ptr node);
}