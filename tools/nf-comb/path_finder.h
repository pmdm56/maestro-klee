#pragma once

#include "assert.h"
#include "bdd/bdd.h"
#include "bdd/nodes/nodes.h"
#include "node_utils.h"

using namespace BDD;

namespace nfcomb {

typedef struct packet_chunk_t {
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> out;
  klee::ref<klee::Expr> size;

  packet_chunk_t(klee::ref<klee::Expr> _in, klee::ref<klee::Expr> _size)
      : in(_in), size(_size) {}

  bool isChanged(const klee::ConstraintManager &constraints);
};

typedef struct bdd_path_t {
  std::vector<Node_ptr> path;
  std::vector<packet_chunk_t> packet;
  std::vector<klee::ConstraintManager> path_constraints;
  int from_id;
  int packet_layer;
  int condition_layer;

  bdd_path_t() : packet_layer(-1), condition_layer(0) {
    klee::ConstraintManager c;
    path_constraints.emplace_back(c);
  }

  bdd_path_t(const BDD::BDD &_bdd)
      : condition_layer(0), packet_layer(-1), from_id(_bdd.get_id()) {
    klee::ConstraintManager c;
    path_constraints.emplace_back(c);
  }

  bdd_path_t(std::shared_ptr<bdd_path_t> &_p)
      : from_id(_p->from_id), packet_layer(_p->packet_layer),
        condition_layer(_p->condition_layer) {

    for (auto n : _p->path)
      path.emplace_back(n);

    for (auto c : _p->packet)
      packet.emplace_back(c);

    for (auto c : _p->path_constraints)
      path_constraints.emplace_back(c);
  }

  std::string dump();
  bool was_packet_modified();
  klee::ConstraintManager get_path_constraints();
  klee::ConstraintManager get_constraints_by_layer(int layer);
  static void build_constraint_layer(std::shared_ptr<bdd_path_t> p);
};

typedef std::shared_ptr<bdd_path_t> bdd_path_ptr;

class PathFinder {
private:
  static bool explore(const Node_ptr &node, bdd_path_ptr p,
                      std::vector<bdd_path_ptr> &paths);
  static bool choose_return_init(bdd_path_ptr p1, bdd_path_ptr p2);
  static std::vector<std::vector<int>>
  check_chunks_alignment(std::vector<packet_chunk_t> p1,
                         std::vector<packet_chunk_t> p2);

public:
  enum PathType { INIT, PROCESS };
  static void get_paths_process(BDD::BDD &bdd,
                                std::vector<bdd_path_ptr> &paths);
  static void get_paths_init(BDD::BDD &bdd, std::vector<bdd_path_ptr> &paths);
  static bdd_path_ptr merge_paths(bdd_path_ptr p1, bdd_path_ptr p2,
                                  PathType type);
  void debug_path(bdd_path_ptr p);
  static std::string dumpVector(const std::vector<int>& vec);
};

} // namespace nfcomb