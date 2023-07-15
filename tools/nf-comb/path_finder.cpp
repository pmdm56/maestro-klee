#include "path_finder.h"

namespace nfcomb {

bool packet_chunk_t::isChanged(const klee::ConstraintManager &constraints) {
  auto eq = kutil::solver_toolbox.exprBuilder->Eq(in, out);
  return !kutil::solver_toolbox.is_expr_always_true(constraints, eq);
}

std::string bdd_path_t::dump() {
  std::stringstream ss;
  ss << "Path -> Len(" << path.size() << ") Constr(" << path_constraints.size()
     << ") Layer(" << packet_layer << ") Packet(" << packet.size()
     << ") Modified(" << (was_packet_modified() ? "yes" : "no") << ")"
     << std::endl;
  return ss.str();
}

bool bdd_path_t::was_packet_modified() {
  // for (auto packet_chunk : packet) {
  //   if (packet_chunk.isChanged(path_constraints))
  //     return true;
  // }
  return false;
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

std::vector<std::vector<int>>
PathFinder::check_chunks_alignment(std::vector<packet_chunk_t> p1,
                                   std::vector<packet_chunk_t> p2) {

  std::vector<std::vector<int>> insertion_order;
  klee::ref<klee::Expr> current_align_size;
  int index1 = 0;
  int index2 = 0;

  while (1) {

    if (index1 == p1.size()) {
      for (index2; index2 < p2.size(); index2++)
        insertion_order.emplace_back((std::vector<int>){1});
      return insertion_order;
    }

    if (index2 == p2.size()) {
      for (index1; index1 < p1.size(); index1++)
        insertion_order.emplace_back((std::vector<int>){0});
      return insertion_order;
    }

    if (kutil::solver_toolbox.isEqual(p1[index1].size, p2[index2].size)) {
      insertion_order.emplace_back((std::vector<int>){0});
      index2++;
      index1++;
      continue;
    }

    if (kutil::solver_toolbox.isGreaterthan(p1[index1].size, p2[index2].size)) {

      current_align_size = p2[index2].size;
      auto align_set = std::vector<int>();
      align_set.emplace_back(1);

      while (
          !kutil::solver_toolbox.isEqual(current_align_size, p1[index1].size)) {
        index2++;
        if (index2 == p2.size())
          return std::vector<std::vector<int>>();
        current_align_size = kutil::solver_toolbox.exprBuilder->Add(
            current_align_size, p2[index2].size);
        align_set.emplace_back(1);
        if (kutil::solver_toolbox.isEqual(current_align_size, p1[index1].size))
          break;
        if (kutil::solver_toolbox.isGreaterthan(current_align_size,
                                                p1[index1].size))
          return std::vector<std::vector<int>>();
      }
      index1++;
      index2++;
      insertion_order.emplace_back(align_set);
      continue;
    }

    if (kutil::solver_toolbox.isGreaterthan(p2[index2].size, p1[index1].size)) {

      current_align_size = p1[index1].size;
      auto align_set = std::vector<int>();
      align_set.emplace_back(0);

      while (
          !kutil::solver_toolbox.isEqual(current_align_size, p2[index2].size)) {
        index1++;
        if (index1 == p1.size())
          return std::vector<std::vector<int>>();
        current_align_size = kutil::solver_toolbox.exprBuilder->Add(
            current_align_size, p1[index1].size);
        align_set.emplace_back(0);
        if (kutil::solver_toolbox.isEqual(current_align_size, p2[index2].size))
          break;
        if (kutil::solver_toolbox.isGreaterthan(current_align_size,
                                                p2[index2].size))
          return std::vector<std::vector<int>>();
      }
      index2++;
      index1++;
      insertion_order.emplace_back(align_set);
    }
  }
}


std::string PathFinder::dumpVector(const std::vector<int>& vec) {
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

bdd_path_ptr PathFinder::merge_paths(bdd_path_ptr p1, bdd_path_ptr p2,
                                     PathType type = PathType::PROCESS) {

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

      // check packet chunks alignment and insertion order (TODO error if not
      // aligned)
      auto insert_chunk_order = check_chunks_alignment(p1->packet, p2->packet);

      return new_path;
    }
  }

  bdd_path_t::build_constraint_layer(new_path);

  return new_path;
}

} // namespace nfcomb
