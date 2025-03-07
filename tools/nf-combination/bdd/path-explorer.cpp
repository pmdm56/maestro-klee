#include "bdd.h"
#include "nodes/nodes.h"
#include "path-explorer.h"

namespace BDD {

bool PathExplorer::explore(const Node_ptr &node, bdd_path_t* p, std::vector<bdd_path_t*>& paths){
  Node *n = node.get();
  
  if(!n)
    return false;

  switch (n->get_type()) {
    case Node::NodeType::BRANCH:
    {
      auto branch = static_cast<const Branch *>(n);

      bdd_path_t *new_path = new bdd_path_t;
      new_path->initializeFrom(p);

      auto clone =
      std::make_shared<Branch>(branch->get_id(), branch->get_condition(), p->bdd_id, p->bdd_name);

      klee::ConstraintManager current_contrs;
      for(auto c = p->constraints.begin(); c != p->constraints.end(); c++)
        current_contrs.addConstraint(*c);

      std::vector<klee::ConstraintManager> c;
      clone->set_constraints(current_contrs);

      p->constraints.addConstraint(branch->get_condition());
      //p->path.push_back(branch->clone());
      p->path.push_back(clone);

      new_path->constraints.addConstraint(exprBuilder->Not(branch->get_condition()));
      //new_path->path.push_back(branch->clone());
      new_path->path.push_back(clone);

      return explore(branch->get_on_true(), p, paths) &
        explore(branch->get_on_false(), new_path, paths);

    }
    case Node::NodeType::CALL:
    {
      auto call = static_cast<const Call *>(n);

      auto clone =
      std::make_shared<Call>(call->get_id(), call->get_call(),p->bdd_id, p->bdd_name);
      p->path.push_back(clone);

      klee::ConstraintManager current_contrs;
      for(auto c = p->constraints.begin(); c != p->constraints.end(); c++)
        current_contrs.addConstraint(*c);

      std::vector<klee::ConstraintManager> c;
      clone->set_constraints(current_contrs);
  
      if(call->get_call().function_name == "packet_borrow_next_chunk"){
        p->layer++;

        auto in_packet_expr = call->get_call().extra_vars["the_chunk"].second;

        packet_chunk_t new_chunk(in_packet_expr);

        p->packet.push_back(new_chunk);

      } else if (call->get_call().function_name == "packet_return_chunk"){

        auto out_packet_expr = call->get_call().args["the_chunk"].in;
        p->packet[p->layer].out = out_packet_expr;

        /*
        auto query_example = klee::Query(p->constraints, out_packet_expr);

        klee::ref<klee::ConstantExpr> res;

        solver_toolbox.solver->getValue(query_example, res);

        std::string exam;
        res->toString(exam);

        std::cerr << "**\n";
        res->dump();
        std::cerr << "\n" << exam << "\n***" << std::endl;
        */
        p->layer--;
      }

      return explore(node->get_next(), p, paths);

    }
    case Node::NodeType::RETURN_INIT:
    {
      auto ri = static_cast<const ReturnInit *>(n);
      auto clone = std::make_shared<ReturnInit>(ri->get_id(), ri->get_return_value(), p->bdd_id, p->bdd_name);
      klee::ConstraintManager current_contrs;
      for(auto c = p->constraints.begin(); c != p->constraints.end(); c++)
        current_contrs.addConstraint(*c);
      clone->set_constraints(current_contrs);
      p->path.push_back(clone);
      paths.push_back(p);
      return true;
    }
    case Node::NodeType::RETURN_PROCESS:
    {
      auto rp = static_cast<const ReturnProcess *>(n);
      auto clone = std::make_shared<ReturnProcess>(rp->get_id(), rp->get_return_value(), rp->get_return_operation(),p->bdd_id, p->bdd_name);
      klee::ConstraintManager current_contrs;
      for(auto c = p->constraints.begin(); c != p->constraints.end(); c++)
        current_contrs.addConstraint(*c);
      clone->set_constraints(current_contrs);
      p->path.push_back(clone);
      paths.push_back(p);
      return true;
    }
    default:
      assert(false);
      break;
  };
}

void PathExplorer::getPathsProcess(BDD bdd, std::vector<bdd_path_t*>& paths){
  bdd_path_t *first_path = new bdd_path_t(bdd.get_name());
  first_path->bdd_id = bdd.get_id();
  explore(bdd.get_process(), first_path, paths);
}

void PathExplorer::getPathsInit(BDD bdd, std::vector<bdd_path_t*>& paths){
  bdd_path_t *first_path = new bdd_path_t(bdd.get_name());
  first_path->bdd_id = bdd.get_id();
  explore(bdd.get_init(), first_path, paths);
}


//TODO: paths from different nfs!
bool PathExplorer::arePathsCompatible(bdd_path_t* p1, bdd_path_t* p2){

  if(!p1->constraints.size() || !p2->constraints.size()){
    return true;
  }

  auto i = 0;
  klee::ref<klee::Expr> expr_1;
  klee::ref<klee::Expr> expr_2;
  klee::ConstraintManager constraints;
  kutil::RetrieveSymbols symbol_retriever1;
  bool res1;


  for (auto c = p1->constraints.begin(); c != p1->constraints.end(); c++, i++){
    if(!i){
      expr_1 = klee::ref<klee::Expr>(*c);
    } else {
      expr_1 = exprBuilder->And(expr_1, *c);
    }
  }

  i = 0;
  for (auto c = p2->constraints.begin(); c != p2->constraints.end(); c++, i++) {
    if(!i){
      expr_2 = klee::ref<klee::Expr>(*c);
    } else {
      expr_2 = exprBuilder->And(expr_2, *c);
    }
  }

  symbol_retriever1.visit(expr_1);
  auto symbols_expr_1 = symbol_retriever1.get_retrieved();
  kutil::ReplaceSymbols symbol_replacer(symbols_expr_1);

  klee::ref<klee::Expr> evaluate_expr =
      exprBuilder->And(expr_1, symbol_replacer.visit(expr_2));

  auto query = klee::Query(constraints, evaluate_expr);
  kutil::solver_toolbox.solver->mayBeTrue(query, res1);
  //solver_toolbox.solver->mayBeFalse(query, res2);
  //solver_toolbox.solver->evaluate(query, val);

  // paths are compatible if they may be true
  return res1;
}

//return process type & value conflict
bool PathExplorer::is_process_res_type_conflict(bdd_path_t* p1, bdd_path_t* p2){
  auto p1_ret = static_cast<ReturnProcess*>(p1->path.at(p1->path.size() - 1).get());
  auto p2_ret = static_cast<ReturnProcess*>(p2->path.at(p2->path.size() - 1).get());
  if(p1_ret->get_return_operation() != p2_ret->get_return_operation()){
    std::cerr << "-- Packet forwarding conflict" << std::endl;
    std::cerr << "  - path_1: " << p1_ret->dump() << std::endl;
    std::cerr << "  - path_2: " << p2_ret->dump() << std::endl;
    return true;
  } else {
    if(p1_ret->get_return_value() != p2_ret->get_return_value()){
      std::cerr << "-- Packet device forwarding conflict" << std::endl;
      std::cerr << "  - path_1: " << p1_ret->get_return_value() << std::endl;
      std::cerr << "  - path_2: " << p2_ret->get_return_value() << std::endl;
      return true;
    }
  }
  return false;
}

} // namespace BDD