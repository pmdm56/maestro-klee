#pragma once

#include <iostream>
#include <unordered_set>
#include <vector>

#include "load-call-paths.h"
#include "symbol.h"

namespace BDD {

class Node;
class BDDVisitor;

typedef std::shared_ptr<Node> BDDNode_ptr;
typedef uint64_t node_id_t;

class Node {
public:
  enum NodeType { BRANCH, CALL, RETURN_INIT, RETURN_PROCESS, RETURN_RAW };

protected:
  uint64_t id;
  NodeType type;

  BDDNode_ptr next;
  BDDNode_ptr prev;

  klee::ConstraintManager constraints;

public:
  Node(uint64_t _id, NodeType _type, klee::ConstraintManager _constraints)
      : id(_id), type(_type), next(nullptr), prev(nullptr),
        constraints(_constraints) {}

  Node(uint64_t _id, NodeType _type, const BDDNode_ptr &_next,
       const BDDNode_ptr &_prev, klee::ConstraintManager _constraints)
      : id(_id), type(_type), next(_next), prev(_prev),
        constraints(_constraints) {}

  void replace_next(const BDDNode_ptr &_next) {
    assert(_next);
    next = _next;
  }

  void add_next(const BDDNode_ptr &_next) {
    assert(next == nullptr);
    assert(_next);
    next = _next;
  }

  void replace_prev(const BDDNode_ptr &_prev) {
    assert(_prev);
    prev = _prev;
  }

  void add_prev(const BDDNode_ptr &_prev) {
    assert(prev == nullptr);
    assert(_prev);
    prev = _prev;
  }

  void disconnect() {
    prev = nullptr;
    next = nullptr;
  }

  const BDDNode_ptr &get_next() const { return next; }
  const BDDNode_ptr &get_next() { return next; }

  const BDDNode_ptr &get_prev() const { return prev; }
  const BDDNode_ptr &get_prev() { return prev; }

  NodeType get_type() const { return type; }
  uint64_t get_id() const { return id; }

  const klee::ConstraintManager &get_node_constraints() const {
    return constraints;
  }

  klee::ConstraintManager get_constraints() const;

  void set_constraints(const klee::ConstraintManager &_constraints) {
    constraints = _constraints;
  }

  symbols_t get_generated_symbols() const;

  // Get generated symbols, but no further than any of these node IDs
  symbols_t get_generated_symbols(
      const std::unordered_set<uint64_t> &furthest_back_nodes) const;

  virtual symbols_t get_local_generated_symbols() const;

  void update_id(uint64_t new_id);

  static std::string process_call_path_filename(std::string call_path_filename);
  void process_call_paths(std::vector<call_path_t *> call_paths);

  virtual std::vector<uint64_t> get_terminating_node_ids() const {
    if (!next) {
      return std::vector<uint64_t>{id};
    }

    return next->get_terminating_node_ids();
  }

  virtual BDDNode_ptr clone(bool recursive = false) const = 0;
  virtual void recursive_update_ids(uint64_t &new_id) = 0;
  virtual void visit(BDDVisitor &visitor) const = 0;
  virtual std::string dump(bool one_liner = false) const = 0;
  virtual std::string dump_recursive(int lvl = 0) const;

protected:
  friend class Call;
  friend class Branch;
  friend class ReturnRaw;
  friend class ReturnInit;
  friend class ReturnProcess;

  virtual std::string get_gv_name() const {
    std::stringstream ss;
    ss << id;
    return ss.str();
  }

  friend class SymbolFactory;
};
} // namespace BDD