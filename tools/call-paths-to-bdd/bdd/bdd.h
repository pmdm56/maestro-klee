#pragma once

#include "nodes/node.h"
#include "symbol-factory.h"

#include "klee-util.h"

namespace BDD {

class BDDVisitor;

class BDD {
private:
  node_id_t id;

  Node_ptr nf_init;
  Node_ptr nf_process;

public:
  BDD(std::vector<call_path_t *> call_paths) : id(0) {
    kutil::solver_toolbox.build();

    call_paths_t cp(call_paths);
    auto root = populate(cp);

    nf_init = populate_init(root);
    nf_process = populate_process(root);

    rename_symbols();
  }
  BDD() : id(0) { kutil::solver_toolbox.build(); }

  BDD(const BDD &bdd)
      : id(bdd.id), nf_init(bdd.nf_init), nf_process(bdd.nf_process) {}

  BDD(const std::string &file_path) : id(0) {
    kutil::solver_toolbox.build();
    deserialize(file_path);
  }

  BDD &operator=(const BDD &) = default;

  node_id_t get_id() const { return id; }

  void set_id(node_id_t _id) {
    assert(_id >= id);
    id = _id;
  }

  Node_ptr get_init() const { return nf_init; }
  Node_ptr get_process() const { return nf_process; }
  Node_ptr get_node_by_id(node_id_t _id) const;

  BDD clone() const;

  void visit(BDDVisitor &visitor) const;

  void set_init(const Node_ptr &_nf_init) { this->nf_init = _nf_init; }
  void set_process(const Node_ptr &_nf_process) { this->nf_process = _nf_process; }

  void rename_symbols();
  void rename_symbols(Node_ptr node, SymbolFactory &factory);

  void serialize(std::string file_path) const;
  void deserialize(const std::string &file_path);

public:
  friend class CallPathsGroup;
  friend class Call;

private:
  // For deserialization

  Node_ptr
  populate(call_paths_t call_paths,
           klee::ConstraintManager accumulated = klee::ConstraintManager());

  Node_ptr populate_init(const Node_ptr &root);
  Node_ptr populate_process(const Node_ptr &root, bool store = false);
};

} // namespace BDD
