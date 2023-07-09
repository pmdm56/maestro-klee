#include "llvm/Support/CommandLine.h"
#include <list>
#include <fstream>
#include "json.hpp"
#include "bdd/nodes/nodes.h"
#include "bdd/visitor.h"
#include "bdd/visitors/graphviz-generator-comb.h"
#include "bdd/visitors/printer-debug.h"
#include "bdd/path-explorer.h"

namespace {

//input BDD files & output BDD
llvm::cl::opt<std::string> BDD1("bdd1", llvm::cl::Required, 
  llvm::cl::desc("First bdd"), llvm::cl::value_desc("<file>.bdd"));
llvm::cl::opt<std::string> BDD2("bdd2", llvm::cl::Required, 
  llvm::cl::desc("Second bdd"), llvm::cl::value_desc("<file>.bdd"));
llvm::cl::opt<std::string> OUT_FILE("out", llvm::cl::Required, 
  llvm::cl::desc("Output file name"), llvm::cl::value_desc("name"));
llvm::cl::opt<std::string> CONFIG("config", llvm::cl::Required, 
  llvm::cl::desc("Configuration file"), llvm::cl::value_desc("<file>.json"));

} // namespace

struct combination_config {
  int conflict_matrix[3][3];
  int prior_changes;
  bool enable_gviz;
  std::string bdd1_color;
  std::string bdd2_color;
  std::string file_name;
};

/**
 * Clone a node, but without any links to other nodes
 *
 * @param node Shared pointer to a node
 * @return Shared pointer to a clone of the node
 */
BDD::Node_ptr dupNode(BDD::Node_ptr node){
  switch(node->get_type()){
    case BDD::Node::NodeType::BRANCH:
    {
      auto original = static_cast<BDD::Branch*>(node.get());
      auto clone = std::make_shared<BDD::Branch>(original->get_id(), original->get_condition(), original->get_from_id(), original->get_from());
      clone->set_constraints(node->get_constraints());
      return clone;
    }
    case BDD::Node::NodeType::CALL:
    {
      auto original = static_cast<BDD::Call*>(node.get());
      auto clone = std::make_shared<BDD::Call>(original->get_id(), original->get_call(), original->get_from_id(), original->get_from());
      clone->set_constraints(node->get_constraints());
      return clone;
    }
    case BDD::Node::NodeType::RETURN_PROCESS:
    {
      auto original = static_cast<BDD::ReturnProcess*>(node.get());
      auto clone = std::make_shared<BDD::ReturnProcess>(original->get_id(), original->get_return_value(), original->get_return_operation(), original->get_from_id(), original->get_from());
      clone->set_constraints(node->get_constraints());
      return clone;
    }
    case BDD::Node::NodeType::RETURN_INIT:
    {
      auto original = static_cast<BDD::ReturnInit*>(node.get());
      auto clone = std::make_shared<BDD::ReturnInit>(original->get_id(), original->get_return_value(), original->get_from_id(), original->get_from());
      clone->set_constraints(node->get_constraints());
      return clone;
    }
    default:
      assert(false && "Cannot duplicate unkown type of node.");
      break;
  }
}



/**
 * Insert <after> node after <root> node.
 *
 * @param root The node where it will be inserted the new node
 * @param after The node that will be inserted after <root>
 */
void insertAfter(BDD::Node_ptr& root, BDD::Node_ptr& after, bool onTrue = true){
  assert(after);
  assert(root);

  BDD::Node_ptr next_root;
  if(root->get_type() == BDD::Node::NodeType::BRANCH){
    auto branch = static_cast<BDD::Branch*>(root.get());
    next_root = onTrue ? root->get_next() : branch->get_on_false();
  } else {
    next_root = root->get_next();
  }

  //root <-> after
  after->replace_prev(root);
  if(root->get_type() == BDD::Node::NodeType::BRANCH){
    auto branch = static_cast<BDD::Branch*>(root.get());
    if(onTrue) branch->replace_on_true(after); else branch->replace_on_false(after);
  } else {
    root->replace_next(after);
  }

  //after <-> root.next
  if(next_root){
    next_root->replace_prev(after);
    if(after->get_type() == BDD::Node::NodeType::BRANCH){
      auto branch = static_cast<BDD::Branch*>(after.get());
      branch->replace_on_true(next_root);
      branch->replace_on_false(next_root->clone(true));
    } else {
      after->replace_next(next_root);
    }
  }                                             

}


//TODO discuss
bool node_equals(BDD::Node_ptr n1, BDD::Node_ptr n2){

  if(n1->get_type() != n2->get_type())
    return false;
  
/* 
  std::cerr << "1\n";
  if(n1->get_constraints()[0].size() != n2->get_constraints()[0].size())
    return false;

  auto c_n1 = n1->get_constraints()[0].begin();
  auto c_n2 = n2->get_constraints()[0].begin();

  for(c_n1, c_n2; c_n1 != n1->get_constraints()[0].end(); c_n1++, c_n2++)
    if(!BDD::solver_toolbox.are_exprs_always_equal(*c_n1, *c_n2))
      return false;

  std::cerr << "2\n"; */

  switch(n2->get_type()){
    case BDD::Node::NodeType::RETURN_PROCESS:
    {
      auto rp1 = static_cast<BDD::ReturnProcess*>(n1.get());
      auto rp2 = static_cast<BDD::ReturnProcess*>(n2.get());

      if(rp1->get_return_operation() != rp2->get_return_operation())
        return false;

      if(rp1->get_return_operation() == BDD::ReturnProcess::Operation::FWD)
        return rp1->get_return_value() == rp2->get_return_value();

      return true;

    }
    case BDD::Node::NodeType::RETURN_INIT:
    {
      auto rp1 = static_cast<BDD::ReturnInit*>(n1.get());
      auto rp2 = static_cast<BDD::ReturnInit*>(n2.get());

      return rp1->get_return_value() == rp2->get_return_value();

    }
    case BDD::Node::NodeType::BRANCH:
    {

      auto b1 = static_cast<BDD::Branch*>(n1.get());
      auto b2 = static_cast<BDD::Branch*>(n2.get());

      //TODO: branch from packet and from data structures
      return  kutil::solver_toolbox.are_exprs_always_equal(b1->get_condition(), b2->get_condition());
    }
    case BDD::Node::NodeType::CALL:
    {
      auto c1 = static_cast<BDD::Call*>(n1.get());
      auto c2 = static_cast<BDD::Call*>(n2.get());

      if(c1->get_call().function_name == c2->get_call().function_name)
      {
        //global
        if(c1->get_call().function_name == "packet_borrow_next_chunk")
          return kutil::solver_toolbox.are_exprs_always_equal(
              c1->get_call().args["length"].expr,
              c2->get_call().args["length"].expr);
    
        //global
        if(c1->get_call().function_name == "packet_return_chunk"){
            auto out_c1 = c1->get_call().args["the_chunk"].in;
            auto out_c2 = c2->get_call().args["the_chunk"].in;
            return out_c1->getWidth() == out_c2->getWidth();
        }
      }

      return  kutil::solver_toolbox.are_calls_equal(c1->get_call(), c2->get_call()) &&
              c1->get_from_id() == c2->get_from_id();
    }
    default:
      assert(false && "Cannot compare two nodes with unkown types.");
      break;
  }
}



//return process being added before another return process
void addNode(BDD::Node_ptr& root, BDD::Node_ptr& new_node){

  if(node_equals(root, new_node)){
    return;
  }

  //prevent duplicate return process
  if(root->get_next())
    if(node_equals(root->get_next(), new_node)){
      return;
    }

  switch (root->get_type()) {
    case BDD::Node::NodeType::BRANCH: {
      
      auto branch = static_cast<BDD::Branch *>(root.get());
      klee::ConstraintManager on_true_path_contrs;
      on_true_path_contrs.addConstraint(branch->get_condition());
      klee::ConstraintManager on_false_path_contrs;
      on_false_path_contrs.addConstraint( kutil::solver_toolbox.exprBuilder->Not(branch->get_condition()));
      auto new_node_clone = dupNode(new_node);

      // on true
      if (new_node->get_constraints().empty() || kutil::solver_toolbox.are_constraints_compatible(on_true_path_contrs,
                                                        new_node->get_constraints())) {                                                
        auto next_root = branch->get_on_true();
        if(!next_root || (next_root->get_type() == BDD::Node::NodeType::RETURN_PROCESS) || (next_root->get_type() == BDD::Node::NodeType::RETURN_INIT)){                   
          insertAfter(root, new_node);
        } else {
          addNode(next_root, new_node);
        }
      }

      // on false
      if (new_node_clone->get_constraints().empty() || kutil::solver_toolbox.are_constraints_compatible(on_false_path_contrs,
                                                        new_node_clone->get_constraints())) {                                                      
        auto next_root = branch->get_on_false();
        if(!next_root || (next_root->get_type() == BDD::Node::NodeType::RETURN_PROCESS) || (next_root->get_type() == BDD::Node::NodeType::RETURN_INIT)){
          insertAfter(root, new_node_clone, false);
        } else {
          addNode(next_root, new_node_clone);
        }
      }
      break;
    }
    case BDD::Node::NodeType::CALL:
    {
      auto call = static_cast<BDD::Call *>(root.get());
      auto next_root = call->get_next();
      if(!next_root || (next_root->get_type() == BDD::Node::NodeType::RETURN_PROCESS) || (next_root->get_type() == BDD::Node::NodeType::RETURN_INIT)){
        insertAfter(root, new_node, false);
      } else {
        addNode(next_root, new_node);
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

void resolveReturnProcessConflits(BDD::bdd_path_t* p1, BDD::bdd_path_t* p2, combination_config conf){

  auto p1_ret = static_cast<BDD::ReturnProcess*>(p1->path.at(p1->path.size() - 1).get());
  auto p2_ret = static_cast<BDD::ReturnProcess*>(p2->path.at(p2->path.size() - 1).get());

  auto res = conf.conflict_matrix[p1_ret->get_return_operation()][p2_ret->get_return_operation()];
  if(res){
    p1_ret->set_valid(false);
    p2_ret->set_valid(true);
  } else {
    p1_ret->set_valid(true);
    p2_ret->set_valid(false);
  }
}

void resolveReturnInitConflits(BDD::bdd_path_t* p1, BDD::bdd_path_t* p2){

  auto node =  p1->path.at(p1->path.size() - 1);
  auto node2 = p2->path.at(p2->path.size() - 1);

  if(node->get_type() != BDD::Node::NodeType::RETURN_INIT || node2->get_type() != BDD::Node::NodeType::RETURN_INIT)
    return;

  auto p1_ret = static_cast<BDD::ReturnInit*>(p1->path.at(p1->path.size() - 1).get());
  auto p2_ret = static_cast<BDD::ReturnInit*>(p2->path.at(p2->path.size() - 1).get());

  if (p2_ret->get_return_value() == BDD::ReturnInit::ReturnType::FAILURE) {
    p1->path.pop_back();
  } else { 
    p2->path.pop_back();
  }
}

bool isPacketBorrow(BDD::Node_ptr node){

  if(node->get_type() != BDD::Node::NodeType::CALL)
    return false;

  auto call = static_cast<BDD::Call*>(node.get());

  return call->get_call().function_name == "packet_borrow_next_chunk";
}

bool isPacketReturn(BDD::Node_ptr node){

  if(node->get_type() != BDD::Node::NodeType::CALL)
    return false;

  auto call = static_cast<BDD::Call*>(node.get());

  return call->get_call().function_name == "packet_return_chunk";
}


bool isGreaterthan(klee::ref<klee::Expr> len1, klee::ref<klee::Expr> len2){

  klee::ref<klee::Expr> expr = kutil::solver_toolbox.exprBuilder->Ule(len1, len2);
  klee::ref<klee::Expr> expr2 = kutil::solver_toolbox.exprBuilder->Not(expr);

  return kutil::solver_toolbox.is_expr_always_true(expr2);
}

bool isEqual(klee::ref<klee::Expr> len1, klee::ref<klee::Expr> len2){

  klee::ref<klee::Expr> expr = kutil::solver_toolbox.exprBuilder->Eq(len1, len2);

  return kutil::solver_toolbox.is_expr_always_true(expr);

}


//TODO if one of the bdds dont use borrows
std::vector<BDD::Node_ptr> mergePaths(BDD::bdd_path_t* p1, BDD::bdd_path_t* p2){

    std::vector<BDD::Node_ptr> merged_path;
    std::vector<int> return_order;
    klee::ref<klee::Expr> len1;
    klee::ref<klee::Expr> len2;
    auto p1_node = p1->path.begin();
    auto p2_node = p2->path.begin();

    ////std::cerr << "a" << std::endl;

    for(p1_node; p1_node != (p1->path.end()-1); p1_node++){
      if(isPacketBorrow(*p1_node)){
        auto borrow1 = static_cast<BDD::Call*>((*p1_node).get());
        if(len1.isNull()){
          len1 = borrow1->get_call().args["length"].expr; 
        } else {
          len1 = kutil::solver_toolbox.exprBuilder->Add(len1, borrow1->get_call().args["length"].expr);
        }

        if(!len2.isNull())
          if(isGreaterthan(len2,len1))
            assert(false && "Packet chunks not aligned.");
        
        //std::cerr << "p1 borrow ";
        //len1->dump();

        if(p2_node != (p2->path.end()-1)){

          while(!isPacketBorrow(*p2_node)){
            merged_path.push_back(*p2_node);
            p2_node++;
            if((p2_node == (p2->path.end()-1)) || isPacketReturn(*p2_node))
              break;
          }

          if(isPacketBorrow(*p2_node)){
            
            auto borrow2 = static_cast<BDD::Call*>((*p2_node).get());

            if(len2.isNull()){
              len2 = borrow2->get_call().args["length"].expr; 
            } else {
              len2 = kutil::solver_toolbox.exprBuilder->Add(len2, borrow2->get_call().args["length"].expr);
            }

            //std::cerr << "p2 borrow ";
            //len2->dump();

            while(isGreaterthan(len1, len2)){
              
              //std::cerr << "aligning p2 w/ p1 size..." << std::endl;

              while(!isPacketBorrow(*p2_node)){
                merged_path.push_back(*p2_node);
                p2_node++;
                if((p2_node == (p2->path.end()-1)) || isPacketReturn(*p2_node))
                  break;
              }

              if(isPacketBorrow(*p2_node)){
                
                auto borrow2 = static_cast<BDD::Call*>((*p2_node).get());
                len2 = kutil::solver_toolbox.exprBuilder->Add(len2, borrow2->get_call().args["length"].expr);
                
                merged_path.push_back(*p2_node);
                return_order.push_back(1);

                if(isGreaterthan(len2, len1))
                  assert(false && "Packet chunks not aligned.");

              } else {
                assert(false && "Packet chunks not aligned.");
              }

            }

            if(isEqual(len1, len2)){
                merged_path.push_back(*p1_node);
                return_order.push_back(0);  
                len1 = nullptr;
                len2 = nullptr;
                p2_node++;
                //std::cerr << "they're equal, skipping to the next pair..." << std::endl;
            } else if (isGreaterthan(len2,len1)){
                merged_path.push_back(*p1_node);
                return_order.push_back(0);  
                len2 = nullptr;
                //std::cerr << "aligning l1 with l2 size..." << std::endl;
            }

          } else {
            merged_path.push_back(*p1_node);
            return_order.push_back(0);
          }
        } else {
          merged_path.push_back(*p1_node);
          return_order.push_back(0);
        }

      } else {
        
        if(isPacketReturn(*p1_node))
          break;

        merged_path.push_back(*p1_node);
      }

    }

    //std::cerr << "a\n";

    if(!len2.isNull())
        if(isGreaterthan(len2,len1))
          assert(false && "Packet chunks not aligned.");

    //std::cerr << (p1->path.end() - p1_node) << "\n";
    //std::cerr << (p2->path.end() - p2_node) << "\n";

    while(p1_node != (p1->path.end()-1)){
        if(isPacketReturn(*p1_node))
          break;
        merged_path.push_back(*p1_node);
        p1_node++;
    }

    //std::cerr << "b\n";

    while(p2_node != (p2->path.end()-1)){
        if(isPacketReturn(*p2_node))
          break;
        if(isPacketBorrow(*p2_node))
          return_order.push_back(1);
        merged_path.push_back(*p2_node);
        p2_node++;
    }


    //std::cerr << "ret vec: ";
    //for(auto i : return_order)
     // std::cerr << i << " ";
    //std::cerr << "\n";

    //std::cerr << (p1->path.end() - p1_node) << " & " << (p2->path.end() - p2_node) <<  "\n";

    int offset = 0;
    for(auto ret_order = return_order.rbegin(); ret_order != return_order.rend(); ret_order++){
      if(*ret_order == 1){
        merged_path.push_back(*p2_node);
        p2_node++;
        int diff = ((return_order.size()- p1->packet.size()) - offset);
        if( diff <= 0)
          p1_node++;
      } else {
        merged_path.push_back(*p1_node);
        p1_node++;
        int diff = ((return_order.size()- p2->packet.size()) - offset);
        if( diff <= 0)
          p2_node++;
      }
      offset++;
      //std::cerr << (p1->path.end() - p1_node) << " & " << (p2->path.end() - p2_node) <<  "\n";
    }

    //std::cerr << (*p1_node)->dump() << "\n";
    //std::cerr << (*p2_node)->dump() << "\n";

    //std::cerr << "c\n";

    assert((*p1_node)->get_type() == BDD::Node::NodeType::RETURN_PROCESS);
    assert((*p2_node)->get_type() == BDD::Node::NodeType::RETURN_PROCESS);

    merged_path.push_back((*p1_node)->get_valid() ? *p1_node : *p2_node);

    //std::cerr << "d\n";

    return merged_path;
}

void dumpPath(std::vector<BDD::Node_ptr> p){
  for(auto n : p){
    if(n->get_type() == BDD::Node::NodeType::BRANCH)
      std::cerr << "(condition) from " << n->get_from() << "[" << n->get_from_id() << "]\n";
    if(n->get_type() == BDD::Node::NodeType::CALL){
      auto call = static_cast<BDD::Call*>(n.get());
      std::cerr << call->get_call().function_name << " from "  << n->get_from() << "[" << n->get_from_id() << "]\n";
    }
    if(n->get_type() == BDD::Node::NodeType::RETURN_PROCESS){
      auto rp = static_cast<BDD::ReturnProcess*>(n.get());
      std::cerr << rp->get_return_operation() << " from "  << n->get_from() << "[" << n->get_from_id() << "]\n";
    }
    if(n->get_type() == BDD::Node::NodeType::RETURN_INIT){
      auto rp = static_cast<BDD::ReturnInit*>(n.get());
      std::cerr << rp->get_return_value() << " from "  << n->get_from() << "[" << n->get_from_id() << "]\n";
    }
  }
}

bool compareReturns(BDD::Node_ptr n1, BDD::Node_ptr n2){
  auto r1 = static_cast<BDD::Call*>(n1.get());
  auto r2 = static_cast<BDD::Call*>(n2.get());
}



std::ostream& operator<<(std::ostream& os, const combination_config& c){
  return os << "Configuration: " << c.file_name << std::endl
            << "Graphviz: " << (c.enable_gviz ? "enabled" : "disabled") << std::endl 
            << "BDD1 color: " << c.bdd1_color << std::endl
            << "BDD2 color: " << c.bdd2_color << std::endl
            << "Prior changes: " << (c.prior_changes ? "BDD2" : "BDD1") << std::endl
            << "Conflict matrix: " << std::endl
            << c.conflict_matrix[0][0] << "  "  << c.conflict_matrix[0][1] << "  " << c.conflict_matrix[0][2] <<  std::endl
            << c.conflict_matrix[1][0] << "  "  << c.conflict_matrix[1][1] << "  " << c.conflict_matrix[1][2] <<  std::endl
            << c.conflict_matrix[2][0] << "  "  << c.conflict_matrix[2][1] << "  " << c.conflict_matrix[2][2] <<  std::endl;

}

combination_config parse_configuration(){
  nlohmann::json j;
  std::ifstream config_file(CONFIG);
  auto json_parsed = j.parse(config_file);

  combination_config ret;

  ret.file_name = CONFIG;
  ret.bdd1_color = *json_parsed.find("bdd1_color");
  ret.bdd2_color = *json_parsed.find("bdd2_color");
  ret.enable_gviz = *json_parsed.find("enable_gviz");
  ret.prior_changes = *json_parsed.find("prior_changes");

  auto m = *json_parsed.find("conflict_matrix");
  for(auto line = 0; line < 3; line++)
    for(auto col = 0; col < 3; col++)
      ret.conflict_matrix[line][col] = m[line][col];

  return ret;
}


void createGviz(BDD::BDD bdd, combination_config conf){
  auto file = std::ofstream(OUT_FILE + ".gv");
  assert(file.is_open());
  
  BDD::GraphvizGenerator gv(file, conf.bdd1_color, conf.bdd2_color, BDD1, BDD2);
  gv.set_show_init_graph(true);
  gv.visit(bdd);

  file.close();
 
}

BDD::Node_ptr normalize_init_bdd(BDD::Node_ptr current){

  if (current->get_type() == BDD::Node::NodeType::BRANCH) {
    
    auto branch_node = static_cast<BDD::Branch *>(current.get());
    BDD::Node_ptr r1 = normalize_init_bdd(branch_node->get_on_true());
    BDD::Node_ptr r2 = normalize_init_bdd(branch_node->get_on_false());
    auto ri1 = static_cast<BDD::ReturnInit*>(r1.get());
    auto ri2 = static_cast<BDD::ReturnInit*>(r2.get());

    if(node_equals(r1,r2))
      if(r1->get_type() == BDD::Node::NodeType::RETURN_INIT){
        if(ri1->get_return_value() == BDD::ReturnInit::ReturnType::FAILURE){
          auto previous = current->get_prev();
          if(previous->get_type() == BDD::Node::NodeType::CALL)
            previous->add_next(r1);
          else if(previous->get_type() == BDD::Node::NodeType::BRANCH){
            auto b = static_cast<BDD::Branch *>(previous.get());
            if(node_equals(b->get_on_true(), current))
              b->add_on_true(r1);
            else b->add_on_false(r2);
          }
          return r1;
        }
      }

    branch_node->add_on_false(r2);

    return ri1->get_return_value() == BDD::ReturnInit::ReturnType::FAILURE ?
      r2 : r1;
    
  }

  if(current->get_type() == BDD::Node::NodeType::CALL)
    return normalize_init_bdd(current->get_next());
  
  if(current->get_type() == BDD::Node::NodeType::RETURN_INIT)
    return current;
  
}

void merge_inits(BDD::BDD &new_bdd, BDD::BDD &bdd1, BDD::BDD &bdd2, uint64_t &new_ids){

  bdd1.get_init()->recursive_update_ids(new_ids);
  bdd2.get_init()->recursive_update_ids(++new_ids);

  std::vector<BDD::bdd_path_t*> bdd1_init_paths;
  std::vector<BDD::bdd_path_t*> bdd2_init_paths;
  BDD::PathExplorer explorer;
  auto new_root = new_bdd.get_init();

  explorer.getPathsInit(bdd1, bdd1_init_paths);
  explorer.getPathsInit(bdd2, bdd2_init_paths);

  for(auto p: bdd1_init_paths)
    for(auto p1: bdd2_init_paths){
      resolveReturnInitConflits(p, p1);

      for(auto n: p->path){
        if(!new_root){
          new_bdd.add_init(n);
          new_root = new_bdd.get_init();
        } else addNode(new_root, n);
      }

      for(auto n: p1->path){
        addNode(new_root, n);
      }

    }

  normalize_init_bdd(new_bdd.get_init());
}

void merge_process(BDD::BDD &new_bdd, BDD::BDD &bdd1, BDD::BDD &bdd2, combination_config conf, uint64_t &new_ids){

  bdd1.get_process()->recursive_update_ids(new_ids);
  bdd2.get_process()->recursive_update_ids(++new_ids);

  BDD::PathExplorer explorer;

  std::vector<BDD::bdd_path_t*> bdd1_paths;
  std::vector<BDD::bdd_path_t*> bdd2_paths;
  std::vector<std::vector<BDD::Node_ptr>> returns;

  explorer.getPathsProcess(bdd1, bdd1_paths);
  explorer.getPathsProcess(bdd2, bdd2_paths);

  for(auto p1 : bdd1_paths){
    for(auto p2 : bdd2_paths){
      if(explorer.arePathsCompatible(p1, p2)){

        resolveReturnProcessConflits(p1,p2,conf);

        //alignment check & insertion order
        std::vector<BDD::Node_ptr> new_path = mergePaths(p1,p2);

        for(auto node : new_path){
          if(isPacketReturn(node))
            continue;
          auto root = new_bdd.get_process(); 
          if(!root)
            new_bdd.add_process(node);
          else addNode(root, node);
        } 

        auto ret = new_path.rbegin() + 1;
        int current_return = 0;
        while(isPacketReturn(*ret)){
          if(current_return == returns.size()){
            std::vector<BDD::Node_ptr> new_layer;
            returns.push_back(new_layer);
          }
          returns[current_return].push_back(*ret);
          current_return++;
          ret++;
        }

      }
    }
  }

  auto root = new_bdd.get_process();
  for(auto ret_layer = returns.rbegin(); ret_layer != returns.rend(); ret_layer++)
    for(auto ret : *ret_layer)
      addNode(root, ret);

}

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  combination_config conf = parse_configuration();
  std::cerr << conf;

  kutil::solver_toolbox.build();
  
  BDD::BDD bdd1(BDD1, 0);
  BDD::BDD bdd2(BDD2, 1);
  BDD::BDD new_bdd;

  uint64_t new_ids = 0;
  merge_inits(new_bdd, bdd1, bdd2, new_ids);
  merge_process(new_bdd, bdd1, bdd2, conf, new_ids);

  new_ids = 0;
  //Se if there is dependencies between init and process
  new_bdd.get_init()->recursive_update_ids(new_ids);
  new_bdd.get_process()->recursive_update_ids(++new_ids);


  if(conf.enable_gviz)
    createGviz(new_bdd, conf);

  //TODO nf_init serialize need call paths...
  new_bdd.serialize(OUT_FILE);

  return 0;
}