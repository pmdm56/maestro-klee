#include "nf_comb.h"

namespace {

// input BDD files & output BDD
llvm::cl::opt<std::string> BDD1("bdd1", llvm::cl::Required,
                                llvm::cl::desc("First bdd"),
                                llvm::cl::value_desc("<file>.bdd"));
llvm::cl::opt<std::string> BDD2("bdd2", llvm::cl::Required,
                                llvm::cl::desc("Second bdd"),
                                llvm::cl::value_desc("<file>.bdd"));
llvm::cl::opt<std::string> OUT_FILE("out", llvm::cl::Required,
                                    llvm::cl::desc("Output file name"),
                                    llvm::cl::value_desc("name"));
llvm::cl::opt<std::string> CONFIG("config", llvm::cl::Required,
                                  llvm::cl::desc("Configuration file"),
                                  llvm::cl::value_desc("<file>.json"));
llvm::cl::opt<int> PATHS("paths", llvm::cl::Required);

} // namespace

BDD::Node_ptr normalize_init(BDD::Node_ptr current) {

  switch (current->get_type())
  {
  case Node::NodeType::RETURN_INIT:
  {
    return current;
  }
  case Node::NodeType::CALL:
  {
    return normalize_init(current->get_next());
  }
  case Node::NodeType::BRANCH:
  {  
    auto branch = static_cast<Branch*>(current.get());
    normalize_init(branch->get_on_true());
    auto fail_node = normalize_init(branch->get_on_false());
    branch->add_on_false(fail_node);
    return fail_node;
  }
  default:
    break;
  }
}

void merge_init(BDD::BDD &new_bdd, BDD::BDD &bdd1, BDD::BDD &bdd2, combination_config conf) {

  std::vector<bdd_path_ptr> bdd1_paths;
  std::vector<bdd_path_ptr> bdd2_paths;
  auto new_root = new_bdd.get_init();

  PathFinder::get_paths_init(bdd1, bdd1_paths);
  PathFinder::get_paths_init(bdd2, bdd2_paths);

  for (auto p1 : bdd1_paths)
    for (auto p2 : bdd2_paths) {
      bdd_path_ptr new_path =
          PathFinder::merge_paths(p1, p2, PathFinder::PathType::INIT, conf);
      if (!new_path->path.empty()) {
        auto constr_layer = 0;
        for(auto n: new_path->path){
          if(!new_root){
            new_bdd.set_init(n);
            new_root = new_bdd.get_init();
          } else add_node(new_root, n, new_path->get_constraints_by_layer(constr_layer));
          if(n->get_type() == Node::NodeType::BRANCH)
            constr_layer++;
        }
      }
    }

  normalize_init(new_bdd.get_init());
}

void merge_process(BDD::BDD &new_bdd, BDD::BDD &bdd1, BDD::BDD &bdd2, combination_config conf){

  std::vector<bdd_path_ptr> bdd1_paths;
  std::vector<bdd_path_ptr> bdd2_paths;
  auto new_root = new_bdd.get_process();

  PathFinder::get_paths_process(bdd1, bdd1_paths);
  PathFinder::get_paths_process(bdd2, bdd2_paths);

  for (auto p1 : bdd1_paths)
    for (auto p2 : bdd2_paths) {
      bdd_path_ptr new_path =
          PathFinder::merge_paths(p1, p2, PathFinder::PathType::PROCESS, conf);
      if (!new_path->path.empty()) {
        auto constr_layer = 0;
        for(auto n: new_path->path){
          if(!new_root){
            new_bdd.set_process(n);
            new_root = new_bdd.get_process();
          } else add_node(new_root, n, new_path->get_constraints_by_layer(constr_layer));
          if(n->get_type() == Node::NodeType::BRANCH)
            constr_layer++;
        }
      }
    }
}

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  combination_config conf = parse_configuration(CONFIG);
  std::cerr << conf;

  BDD::BDD bdd1(BDD1);
  BDD::BDD bdd2(BDD2);
  BDD::BDD new_bdd;
  node_id_t new_ids = 0;
  node_id_t new_bdd_ids = 0;

  bdd1.update_node_ids(new_ids);
  bdd2.update_node_ids(++new_ids);

  merge_init(new_bdd, bdd1, bdd2, conf);
  merge_process(new_bdd, bdd1, bdd2, conf);

  new_bdd.update_node_ids(new_bdd_ids);

  if (conf.enable_gviz)
    createGviz(new_bdd, conf, OUT_FILE);

  return 0;
}
