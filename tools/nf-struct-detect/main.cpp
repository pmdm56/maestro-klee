/* -*- mode: c++; c-basic-offset: 2; -*- */

//===-- ktest-dehavoc.cpp ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "nf_comb.h"
#include "llvm/Support/CommandLine.h"

namespace {
llvm::cl::opt<std::string> BDD_FILE("bdd", llvm::cl::Required,
                                    llvm::cl::desc("BDD to analyze"),
                                    llvm::cl::value_desc("<file>.bdd"));

}

// struct bdd_map {
//   klee::ref<klee::Expr> id;
//   klee::ref<klee::Expr> size;
//   std::string eq_func;
//   std::string hash_func;
//   bdd_map(klee::ref<klee::Expr> _id, klee::ref<klee::Expr> _size,
//           std::string _keq, std::string _khash)
//       : id(_id), size(_size), eq_func(_keq), hash_func(_khash) {}
//   friend std::ostream &operator<<(std::ostream &os, const bdd_map &data) {
//     os << "--- [Map] ---" << std::endl;
//     os << "Id: " << kutil::pretty_print_expr(data.id) << std::endl;
//     os << "Size: " << kutil::pretty_print_expr(data.size) << std::endl;
//     os << "Equals function: " << data.eq_func << std::endl;
//     os << "Hash function: " << data.hash_func << std::endl;
//     os << "-------------";
//     return os;
//   }
// };

// struct vector_access {
//   Node_ptr call_borrow;
//   vector_access(Node_ptr _call) : call_borrow(_call) {}
//   klee::ref<klee::Expr> get_index() const {
//     auto c = static_cast<Call *>(call_borrow.get());
//     return c->get_call().args["index"].expr;
//   }
//   klee::ConstraintManager get_constrs() const {
//     auto c = static_cast<Call *>(call_borrow.get());
//     return c->get_constraints();
//   }
//   call_t get_borrow_call() const {
//     auto c = static_cast<Call *>(call_borrow.get());
//     return c->get_call();
//   }
//   friend std::ostream &operator<<(std::ostream &os, const vector_access
//   &data) {
//     os << "-> "
//        <<
//        kutil::pretty_print_expr(data.get_borrow_call().args["vector"].expr)
//        << "[" << kutil::pretty_print_expr(data.get_index()) << "]" <<
//        std::endl;
//     return os;
//   }
// };

// struct bdd_vector {
//   klee::ref<klee::Expr> id;
//   klee::ref<klee::Expr> size;
//   klee::ref<klee::Expr> elem_size;
//   std::vector<vector_access> access;

//   bdd_vector(klee::ref<klee::Expr> _id, klee::ref<klee::Expr> _size,
//              klee::ref<klee::Expr> _elem_size)
//       : id(_id), size(_size), elem_size(_elem_size) {}
//   friend std::ostream &operator<<(std::ostream &os, const bdd_vector &data) {
//     os << "--- [Vector] ---" << std::endl;
//     os << "Id: " << kutil::pretty_print_expr(data.id) << std::endl;
//     os << "Size: " << kutil::pretty_print_expr(data.size) << std::endl;
//     os << "Element size: " << kutil::pretty_print_expr(data.elem_size)
//        << std::endl;
//     os << "Access (" << data.access.size() << " total):" << std::endl;
//     for (auto a : data.access)
//       os << a;
//     os << "----------------";
//     return os;
//   }

//   void merge_with(bdd_vector &other) {

//     // check if theres any valyue with same index (and with compatible
//     // constraints)
//     for (auto v1_access : this->access)
//       for (auto v2_access : other.access)
//         if (kutil::solver_toolbox.are_constraints_compatible(
//                 v1_access.get_constrs(), v2_access.get_constrs()) &&
//             kutil::solver_toolbox.are_exprs_always_equal(
//                 v1_access.get_index(), v2_access.get_index()) &&
//             (v1_access.call_borrow->get_id() >
//              v2_access.call_borrow->get_id())) {
//           std::cerr << "Possible merge found on index "
//                     << kutil::pretty_print_expr(v1_access.get_index())
//                     << std::endl;
//         }
//   }
// };

// std::vector<bdd_map> bdd_maps;
// std::vector<bdd_vector> bdd_vectors;
// std::map<klee::ref<klee::Expr>,
// std::vector<std::vector<klee::ref<klee::Expr>>>>
//     index_vectors;

// void parse_call(Node_ptr node) {

//   auto call_n = static_cast<Call *>(node.get());
//   auto call = call_n->get_call();
//   klee::ConstraintManager c(call_n->get_constraints());

//   if (call.function_name == "map_allocate") {
//     bdd_map map(call.args["map_out"].out, call.args["capacity"].expr,
//                 call.args["keq"].fn_ptr_name.second,
//                 call.args["khash"].fn_ptr_name.second);
//     bdd_maps.emplace_back(map);
//   } else if (call.function_name == "vector_allocate") {
//     bdd_vector vector(call.args["vector_out"].out,
//     call.args["capacity"].expr,
//                       call.args["elem_size"].expr);
//     bdd_vectors.emplace_back(vector);
//   } else if (call.function_name == "vector_borrow") {
//     auto vec_id = call.args["vector"].expr;
//     auto vec = std::find_if(
//         bdd_vectors.begin(), bdd_vectors.end(), [vec_id](const bdd_vector &v)
//         {
//           return kutil::solver_toolbox.are_exprs_always_equal(v.id, vec_id);
//         });
//     if (vec != bdd_vectors.end()) {
//       // (*vec).access.emplace_back(call);
//       vector_access na(node);
//       (*vec).access.emplace_back(na);
//       std::cerr << na;
//       //index_vectors[na.get_index()].at(indi).emplace_back(vec_id);
//     }
//   }
// }

// void explore(bdd_path_ptr p) {

//   for (auto n : p->path)
//     if (n->get_type() == Node::NodeType::CALL)
//       parse_call(n);
// }

struct vec_access {
  klee::ref<klee::Expr> vec;
  klee::ref<klee::Expr> index;
  vec_access(klee::ref<klee::Expr> _vec, klee::ref<klee::Expr> _i)
      : vec(_vec), index(_i) {}
  friend std::ostream &operator<<(std::ostream &os, const vec_access &a) {
    os << kutil::pretty_print_expr(a.vec) << "["
       << kutil::pretty_print_expr(a.index) << "]";
    return os;
  }
};

std::vector<std::vector<vec_access>> access_list;

void search_access(bdd_path_ptr p) {
  std::vector<vec_access> path_accesses;
  for (auto node : p->path)
    if (node->get_type() == Node::NodeType::CALL) {
      auto call_node = static_cast<Call *>(node.get());
      if (call_node->get_call().function_name == "vector_borrow") {
        path_accesses.emplace_back(vec_access(call_node->get_call().args["vector"].expr,
                                 call_node->get_call().args["index"].expr));
      }
    }
  if(!path_accesses.empty())
  access_list.emplace_back(path_accesses);
}

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  BDD::BDD bdd(BDD_FILE);

  std::vector<bdd_path_ptr> paths;
  PathFinder::get_paths_process(bdd, paths);

  for (auto p : paths)
    search_access(p);

  for(auto a: access_list){
    std::cerr << "--------\n";
    for(auto x: a)
      std::cerr << x << " ";
    std::cerr << "\n";
  }

  return 0;
}
