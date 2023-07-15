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

  if (current->get_type() == BDD::Node::NodeType::BRANCH) {

    auto branch_node = static_cast<BDD::Branch *>(current.get());
    BDD::Node_ptr r1 = normalize_init(branch_node->get_on_true());
    BDD::Node_ptr r2 = normalize_init(branch_node->get_on_false());
    auto ri1 = static_cast<BDD::ReturnInit *>(r1.get());
    auto ri2 = static_cast<BDD::ReturnInit *>(r2.get());

    if (node_equals(r1, r2))
      if (r1->get_type() == BDD::Node::NodeType::RETURN_INIT) {
        if (ri1->get_return_value() == BDD::ReturnInit::ReturnType::FAILURE) {
          auto previous = current->get_prev();
          if (previous->get_type() == BDD::Node::NodeType::CALL)
            previous->add_next(r1);
          else if (previous->get_type() == BDD::Node::NodeType::BRANCH) {
            auto b = static_cast<BDD::Branch *>(previous.get());
            if (node_equals(b->get_on_true(), current))
              b->add_on_true(r1);
            else
              b->add_on_false(r2);
          }
          return r1;
        }
      }

    branch_node->add_on_false(r2);

    return ri1->get_return_value() == BDD::ReturnInit::ReturnType::FAILURE ? r2
                                                                           : r1;
  }
}

void merge_init(BDD::BDD &new_bdd, BDD::BDD &bdd1, BDD::BDD &bdd2) {

  std::vector<bdd_path_ptr> bdd1_paths;
  std::vector<bdd_path_ptr> bdd2_paths;
  auto new_root = new_bdd.get_init();

  PathFinder::get_paths_init(bdd1, bdd1_paths);
  PathFinder::get_paths_init(bdd2, bdd2_paths);

  for (auto p1 : bdd1_paths)
    for (auto p2 : bdd2_paths) {
      bdd_path_ptr new_path =
          PathFinder::merge_paths(p1, p2, PathFinder::PathType::INIT);
      auto constr_layer = 0;
      if (!new_path->path.empty()) {
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

  //normalize_init(new_bdd.get_init());
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

  merge_init(new_bdd, bdd1, bdd2);

  new_bdd.update_node_ids(new_bdd_ids);

  if (conf.enable_gviz)
    createGviz(new_bdd, conf, OUT_FILE);

  return 0;
}
