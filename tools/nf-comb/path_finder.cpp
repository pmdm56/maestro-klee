#include "path_finder.h"

namespace nfcomb {

bool packet_chunk_t::isChanged(const klee::ConstraintManager &constraints) {
  auto eq = kutil::solver_toolbox.exprBuilder->Eq(in, out);
  return !kutil::solver_toolbox.is_expr_always_true(constraints, eq);
}

std::string bdd_path_t::dump() {
  std::stringstream ss;
  ss << "Path -> Len(" << path.size() << ") Constr(" << path_constraints.size()
     << ") Layer(" << packet_layer << ") Packet(" << packet.size() << std::endl;
  return ss.str();
}

klee::ConstraintManager bdd_path_t::get_path_constraints() {
  klee::ConstraintManager c;
  for (auto n : path)
    if (n->get_type() == Node::NodeType::BRANCH)
      if (((Branch *)n.get())->get_current_path_branch())
        c.addConstraint(((Branch *)n.get())->get_condition());
      else
        c.addConstraint(kutil::solver_toolbox.exprBuilder->Not(
            ((Branch *)n.get())->get_condition()));

  return c;
}

bool PathFinder::explore(const Node_ptr &node, bdd_path_ptr p,
                         std::vector<bdd_path_ptr> &paths) {

  if (!node)
    return false;

  switch (node->get_type()) {

  case Node::NodeType::BRANCH: {
    auto branch = static_cast<const Branch *>(node.get());
    auto clone_true = dup_node(node);
    auto clone_false = dup_node(node);
    clone_true->set_from_id(p->from_id);
    clone_false->set_from_id(p->from_id);
    bdd_path_ptr p_false = std::make_shared<bdd_path_t>(p);

    ((Branch *)clone_true.get())->set_path_branch(true);
    p->path.emplace_back(clone_true);
    ((Branch *)clone_false.get())->set_path_branch(false);
    p_false->path.emplace_back(clone_false);

    klee::ConstraintManager c_p, c_p_false;
    p->condition_layer++;
    p_false->condition_layer++;

    for (auto c = p->path_constraints[p->condition_layer - 1].begin();
         c != p->path_constraints[p->condition_layer - 1].end(); c++)
      c_p.addConstraint(*c);
    c_p.addConstraint(branch->get_condition());
    p->path_constraints.emplace_back(c_p);

    for (auto c =
             p_false->path_constraints[p_false->condition_layer - 1].begin();
         c != p_false->path_constraints[p_false->condition_layer - 1].end();
         c++)
      c_p_false.addConstraint(*c);
    c_p_false.addConstraint(
        kutil::solver_toolbox.exprBuilder->Not(branch->get_condition()));
    p_false->path_constraints.emplace_back(c_p_false);

    return explore(branch->get_on_true(), p, paths) &
           explore(branch->get_on_false(), p_false, paths);
  }
  case Node::NodeType::CALL: {
    auto call = static_cast<const Call *>(node.get());
    auto clone = dup_node(node);
    clone->set_from_id(p->from_id);
    p->path.emplace_back(clone);

    if (call->get_call().function_name == "packet_borrow_next_chunk") {
      p->packet_layer++;

      auto in_packet_expr = call->get_call().extra_vars["the_chunk"].second;
      auto expr_size = call->get_call().args["length"].expr;

      packet_chunk_t new_chunk(in_packet_expr, expr_size);
      p->packet.emplace_back(new_chunk);

    } else if (call->get_call().function_name == "packet_return_chunk") {

      auto out_packet_expr = call->get_call().args["the_chunk"].in;
      p->packet[p->packet_layer].out = out_packet_expr;
      p->packet_layer--;
    }

    return explore(node->get_next(), p, paths);
  }
  case Node::NodeType::RETURN_INIT: {
    auto return_init = static_cast<const ReturnInit *>(node.get());
    auto clone = dup_node(node);
    clone->set_from_id(p->from_id);
    p->path.emplace_back(clone);
    paths.emplace_back(p);
    return true;
  }
  case Node::NodeType::RETURN_PROCESS: {
    auto return_process = static_cast<const ReturnProcess *>(node.get());
    auto clone = dup_node(node);
    clone->set_from_id(p->from_id);
    p->path.emplace_back(clone);
    paths.emplace_back(p);
    return true;
  }
  default:
    assert(false && "Unkown node. Aborting...");
    break;
  };
}

void PathFinder::debug_path(bdd_path_ptr p) {

  for (auto n : p->path) {
    switch (n->get_type()) {
    case Node::NodeType::BRANCH: {
      if (((Branch *)n.get())->get_current_path_branch())
        std::cerr << kutil::pretty_print_expr(
                         ((Branch *)n.get())->get_condition())
                  << "\n";
      else
        std::cerr << "!"
                  << kutil::pretty_print_expr(
                         ((Branch *)n.get())->get_condition())
                  << "\n";
      break;
    }
    case Node::NodeType::CALL: {
      std::cerr << ((Call *)n.get())->get_call().function_name << "\n";
      break;
    }
    case Node::NodeType::RETURN_INIT: {
      std::cerr << n->dump() << "\n";
      break;
    }
    case Node::NodeType::RETURN_PROCESS: {
      std::cerr << n->dump() << "\n";
      break;
    }
    }
  }
}

void PathFinder::get_paths_process(BDD::BDD &bdd,
                                   std::vector<bdd_path_ptr> &paths) {
  bdd_path_ptr first_path = std::make_shared<bdd_path_t>(bdd);
  auto root_process = bdd.get_process();

  explore(root_process, first_path, paths);
}

void PathFinder::get_paths_init(BDD::BDD &bdd,
                                std::vector<bdd_path_ptr> &paths) {
  bdd_path_ptr first_path = std::make_shared<bdd_path_t>(bdd);
  auto root_init = bdd.get_init();
  explore(root_init, first_path, paths);
}

bool PathFinder::choose_return_init(bdd_path_ptr p1, bdd_path_ptr p2) {

  auto node = p1->path.at(p1->path.size() - 1);
  auto node2 = p2->path.at(p2->path.size() - 1);

  auto p1_ret =
      static_cast<BDD::ReturnInit *>(p1->path.at(p1->path.size() - 1).get());
  auto p2_ret =
      static_cast<BDD::ReturnInit *>(p2->path.at(p2->path.size() - 1).get());

  if (p2_ret->get_return_value() == BDD::ReturnInit::ReturnType::FAILURE) {
    return true;
  } else {
    return false;
  }
}

klee::ConstraintManager bdd_path_t::get_constraints_by_layer(int layer) {
  if (layer >= path_constraints.size())
    return klee::ConstraintManager();
  else
    return path_constraints[layer];
}

void bdd_path_t::build_constraint_layer(std::shared_ptr<bdd_path_t> p) {
  for (auto n : p->path)
    if (n->get_type() == Node::BRANCH) {
      klee::ConstraintManager c_p;
      p->condition_layer++;
      for (auto c = p->path_constraints[p->condition_layer - 1].begin();
           c != p->path_constraints[p->condition_layer - 1].end(); c++)
        c_p.addConstraint(*c);
      if (((Branch *)n.get())->get_current_path_branch())
        c_p.addConstraint(((Branch *)n.get())->get_condition());
      else
        c_p.addConstraint(kutil::solver_toolbox.exprBuilder->Not(
            ((Branch *)n.get())->get_condition()));
      p->path_constraints.emplace_back(c_p);
    }
}

bool is_bit_byte_modified(packet_chunk_t &c, klee::Expr::Width bit,
                          klee::ConstraintManager constrs) {
  auto in_bit =
      kutil::solver_toolbox.exprBuilder->Extract(c.in, bit, klee::Expr::Bool);
  auto out_bit =
      kutil::solver_toolbox.exprBuilder->Extract(c.out, bit, klee::Expr::Bool);
  auto eq_bit = kutil::solver_toolbox.exprBuilder->Eq(in_bit, out_bit);
  auto always_eq = kutil::solver_toolbox.is_expr_always_true(constrs, eq_bit);

  return !always_eq;
}

// just for perfect alignment because they have the same size
klee::ref<klee::Expr> resolve_write_conflicts(packet_chunk_t &p1, packet_chunk_t &p2,
                             klee::ConstraintManager &constraints,
                             combination_config conf) {

  auto witdh = p1.out.get()->getWidth(); // bits (TODO precision bits / bytes)
  klee::ref<klee::Expr> new_return_expr;
  klee::ref<klee::Expr> choosen_bit;

  for (auto bit = 0u; bit < witdh; bit++) {

    if (is_bit_byte_modified(p1, bit, constraints) &&
        is_bit_byte_modified(p2, bit, constraints)) {

      if (!conf.available)
        ERROR_MSG("A conflict has been detected between chunks. Aborting...");

      choosen_bit = conf.prior_changes
                        ? kutil::solver_toolbox.exprBuilder->Extract(
                              p2.out, bit, klee::Expr::Bool)
                        : kutil::solver_toolbox.exprBuilder->Extract(
                              p1.out, bit, klee::Expr::Bool);
    } else if (is_bit_byte_modified(p1, bit, constraints)) {
      choosen_bit = kutil::solver_toolbox.exprBuilder->Extract(
          p1.out, bit, klee::Expr::Bool);
    } else {
      choosen_bit = kutil::solver_toolbox.exprBuilder->Extract(
          p2.out, bit, klee::Expr::Bool);
    }

    new_return_expr = new_return_expr.isNull()
                          ? choosen_bit
                          : kutil::solver_toolbox.exprBuilder->Concat(
                                new_return_expr, choosen_bit);
  }

  return new_return_expr;
}

chunk_order_ret PathFinder::check_chunks_alignment(
    std::vector<packet_chunk_t> p1, std::vector<packet_chunk_t> p2,
    klee::ConstraintManager &constraints, combination_config conf) {
  
  chunk_order_ret ret;
  int index1 = 0;
  int index2 = 0;
  klee::ref<klee::Expr> current_align_size;

  while (1) {

    if (index1 == p1.size()) {
      for (index2; index2 < p2.size(); index2++){
        ret.order.emplace_back((std::vector<int>){1});
        packet_chunk_t p(p2[index2].in, p2[index2].size, p2[index2].out);
        ret.new_packet.emplace_back(p);
      }
      return ret;
    }

    if (index2 == p2.size()) {
      for (index1; index1 < p1.size(); index1++){
        ret.order.emplace_back((std::vector<int>){0});
        packet_chunk_t p(p1[index1].in, p1[index1].size, p1[index1].out);
        ret.new_packet.emplace_back(p);
      }
      return ret;
    }

    if (kutil::solver_toolbox.isEqual(p1[index1].size, p2[index2].size,
                                      constraints)) {
      auto new_chunk = resolve_write_conflicts(p1[index1], p1[index2], constraints, conf);
      ret.order.emplace_back((std::vector<int>){0});
      packet_chunk_t p(p1[index1].in, p1[index1].size, new_chunk);
      ret.new_packet.emplace_back(p);
      index2++;
      index1++;
      continue;
    }

    if (kutil::solver_toolbox.isGreaterthan(p1[index1].size, p2[index2].size, constraints)) {

      current_align_size = p2[index2].size;
      auto align_set = std::vector<int>();
      align_set.emplace_back(1);
      auto temp_p1_offset = 0u;

      while (
          !kutil::solver_toolbox.isEqual(current_align_size, p1[index1].size, constraints)) {
        index2++;
        if (index2 == p2.size())
          ERROR_MSG("Packet chunks not aligned.");

        current_align_size = kutil::solver_toolbox.exprBuilder->Add(
            current_align_size, p2[index2].size);
        align_set.emplace_back(1);

        packet_chunk_t temp_p1(p2[index2].in, p2[index2].size);
        temp_p1.out = kutil::solver_toolbox.exprBuilder->Extract(p1[index1].out, temp_p1_offset, p2[index2].out.get()->getWidth());
        temp_p1_offset += p2[index2].out.get()->getWidth();
        auto new_chunk = resolve_write_conflicts(temp_p1, p2[index2], constraints, conf);
        packet_chunk_t p(p2[index2].in, p2[index2].size, new_chunk);
        ret.new_packet.emplace_back(p);

        if (kutil::solver_toolbox.isEqual(current_align_size, p1[index1].size, constraints))
          break;
        if (kutil::solver_toolbox.isGreaterthan(current_align_size,
                                                p1[index1].size, constraints))
          ERROR_MSG("Packet chunks not aligned.");
      }
      index1++;
      index2++;
      ret.order.emplace_back(align_set);
      continue;
    }

    if (kutil::solver_toolbox.isGreaterthan(p2[index2].size, p1[index1].size, constraints)) {

      current_align_size = p1[index1].size;
      auto align_set = std::vector<int>();
      align_set.emplace_back(0);
      auto temp_p2_offset = 0u;

      while (
          !kutil::solver_toolbox.isEqual(current_align_size, p2[index2].size, constraints)) {
        index1++;
        if (index1 == p1.size())
          ERROR_MSG("Packet chunks not aligned.");
        current_align_size = kutil::solver_toolbox.exprBuilder->Add(
            current_align_size, p1[index1].size);
        align_set.emplace_back(0);

        packet_chunk_t temp_p2(p1[index1].in, p1[index1].size);
        temp_p2.out = kutil::solver_toolbox.exprBuilder->Extract(p2[index2].out, temp_p2_offset, p1[index1].out.get()->getWidth());
        temp_p2_offset += p1[index1].out.get()->getWidth();
        auto new_chunk = resolve_write_conflicts(temp_p2, p1[index1], constraints, conf);
        packet_chunk_t p(p1[index1].in, p1[index1].size, new_chunk);
        ret.new_packet.emplace_back(p);

        if (kutil::solver_toolbox.isEqual(current_align_size, p2[index2].size, constraints))
          break;
        if (kutil::solver_toolbox.isGreaterthan(current_align_size,
                                                p2[index2].size, constraints))
          ERROR_MSG("Packet chunks not aligned.");
      }
      index2++;
      index1++;
      ret.order.emplace_back(align_set);
    }
  }
   
}

std::string PathFinder::dumpVector(const std::vector<int> &vec) {
  std::stringstream ss;
  ss << "{";

  if (!vec.empty()) {
    ss << vec[0];

    for (size_t i = 1; i < vec.size(); ++i) {
      ss << "," << vec[i];
    }
  }

  ss << "}";
  return ss.str();
}

std::vector<Node_ptr>
PathFinder::get_return_chunks(bdd_path_ptr p1, bdd_path_ptr p2,
                              chunk_order_ret chunk_conf) {

  std::vector<BDD::Node_ptr> return_chunk_nodes;
  auto current_p1_node = p1->path.rbegin() + 1;
  auto current_p2_node = p2->path.rbegin() + 1;
  auto current_chunk_expr = 0;

  for (auto layer : chunk_conf.order) {
    if (layer[0]) {
      for (auto i = 0; i < layer.size(); i++) {
        auto call_node = static_cast<Call *>((*current_p2_node).get());
        auto call = call_node->get_call();
        auto& arg_chunk = call.args["the_chunk"];
        arg_chunk.in = chunk_conf.new_packet[current_chunk_expr].out;
        call_node->set_call(call);
        return_chunk_nodes.emplace_back(*current_p2_node);
        current_p2_node++;
        current_chunk_expr++;
      }
      current_p1_node++;
    } else {
      for (auto i = 0; i < layer.size(); i++) {
        auto call_node = static_cast<Call *>((*current_p1_node).get());
        auto call = call_node->get_call();
        auto& arg_chunk = call.args["the_chunk"];
        arg_chunk.in = chunk_conf.new_packet[current_chunk_expr].out;
        call_node->set_call(call);
        return_chunk_nodes.emplace_back(*current_p1_node);
        current_p1_node++;
        current_chunk_expr++;
      }
      current_p2_node++;
    }
  }

  std::reverse(return_chunk_nodes.begin(), return_chunk_nodes.end());

  return return_chunk_nodes;
}

Node_ptr PathFinder::get_return_process(bdd_path_ptr p1, bdd_path_ptr p2,
                                        combination_config conf) {

  auto p1_ret = static_cast<BDD::ReturnProcess *>(p1->path.back().get());
  auto p2_ret = static_cast<BDD::ReturnProcess *>(p2->path.back().get());

  if (!node_equals(p1->path.back(), p2->path.back()))
    if (!conf.available)
      ERROR_MSG("A conflict has been detected between return process nodes. "
                "Aborting...");

  auto new_ret_choice = conf.conflict_matrix[p1_ret->get_return_operation()]
                                            [p2_ret->get_return_operation()];

  auto new_ret = std::make_shared<ReturnProcess>(
      0,
      (new_ret_choice ? p2_ret->get_return_value()
                      : p1_ret->get_return_value()),
      (new_ret_choice ? p2_ret->get_return_operation()
                      : p1_ret->get_return_operation()),
      (new_ret_choice ? p2_ret->get_from_id() : p1_ret->get_from_id()));

  return new_ret;
}

void add_nodes_until_borrow(std::vector<BDD::Node_ptr>::iterator &current,
                            bdd_path_ptr &new_path, int &current_layer,
                            int &actual_layer) {
  for (current; !isPacketReturn(*current); current++) {
    if (isPacketBorrow(*current)) {
      if (current_layer == actual_layer) {
        new_path->path.emplace_back(*current);
        current++;
        current_layer++;
        break;
      }
      current_layer++;
    } else {
      new_path->path.emplace_back(*current);
    }
  }
}

void PathFinder::add_nodes_from_paths(bdd_path_ptr new_path, bdd_path_ptr p1,
                                      bdd_path_ptr p2,
                                      std::vector<std::vector<int>> order) {
  auto current_p1_node = p1->path.begin();
  auto current_p2_node = p2->path.begin();
  auto p2_layer = 0;
  auto p1_layer = 0;

  for (auto layer = 0; layer < order.size(); layer++)
    for (auto sub_layer = 0; sub_layer < order[layer].size(); sub_layer++)
      if (order[layer][sub_layer])
        add_nodes_until_borrow(current_p2_node, new_path, p2_layer, layer);
      else
        add_nodes_until_borrow(current_p1_node, new_path, p1_layer, layer);

  std::cerr << "--------------\n";

  for (current_p1_node; current_p1_node != p1->path.end(); current_p1_node++)
    if (isPacketBorrow(*current_p1_node))
      continue;
    else if(isPacketReturn(*current_p1_node))
      break;
    else
      new_path->path.emplace_back(*current_p1_node);

  for (current_p2_node; (current_p2_node != p2->path.end()); current_p2_node++)
    if (isPacketBorrow(*current_p2_node))
      continue;
    else if(isPacketReturn(*current_p2_node))
      break;
    else
      new_path->path.emplace_back(*current_p2_node);
}

bdd_path_ptr PathFinder::merge_paths(bdd_path_ptr p1, bdd_path_ptr p2,
                                     PathType type, combination_config conf) {

  bdd_path_ptr new_path = std::make_shared<bdd_path_t>();
  auto p1_constraints = p1->get_path_constraints();
  auto p2_constraints = p2->get_path_constraints();

  if (kutil::solver_toolbox.are_constraints_compatible(p1_constraints,
                                                       p2_constraints)) {

    // nf_init
    if (type == PathType::INIT) {
      for (auto n : p1->path)
        if (n->get_type() == Node::RETURN_INIT)
          continue;
        else
          new_path->path.emplace_back(n);

      for (auto n : p2->path)
        if (n->get_type() == Node::RETURN_INIT)
          continue;
        else
          new_path->path.emplace_back(n);

      if (choose_return_init(p1, p2)) {
        new_path->path.emplace_back(p2->path.back());
      } else
        new_path->path.emplace_back(p1->path.back());
    }

    // nf_process
    if (type == PathType::PROCESS) {

      klee::ConstraintManager new_path_c;
      for (auto c : p1->get_path_constraints())
        new_path_c.addConstraint(c);
      for (auto c : p2->get_path_constraints())
        new_path_c.addConstraint(c);

      // check packet chunks alignment and insertion order (TODO error if not
      // aligned)
      auto insert_chunk_order =
          check_chunks_alignment(p1->packet, p2->packet, new_path_c, conf);

      // merge both path nodes until packet return layer appears (1/3)
      PathFinder::add_nodes_from_paths(new_path, p1, p2, insert_chunk_order.order);

      // get return_chunk nodes (in correct order to insert) (2/3)
      auto new_return_chunks = get_return_chunks(p1, p2, insert_chunk_order);
      new_path->path.insert(new_path->path.end(), new_return_chunks.begin(),
                            new_return_chunks.end());

      // get return_process node (3/3)
      auto new_return_process = PathFinder::get_return_process(p1, p2, conf);
      new_path->path.emplace_back(new_return_process);
    }
  }

  bdd_path_t::build_constraint_layer(new_path);
  return new_path;
}

} // namespace nfcomb
