#pragma once

#include "call-paths-to-bdd.h"
#include "klee-util.h"

#include <assert.h>
#include <unordered_map>

namespace BDD {
namespace emulation {

typedef uint32_t gbps_t;
typedef uint64_t time_us_t;
typedef uint64_t time_ns_t;
typedef float hit_rate_t;

class DataStructure;
typedef std::shared_ptr<DataStructure> DataStructureRef;

struct state_t {
  std::unordered_map<addr_t, DataStructureRef> data_structures;

  void add(const DataStructureRef &ds) {
    auto obj = ds->get_obj();
    assert(data_structures.find(obj) == data_structures.end());
    data_structures[obj] = ds;
  }

  DataStructureRef get(addr_t obj) const {
    assert(data_structures.find(obj) != data_structures.end());
    return data_structures.at(obj);
  }
};

typedef klee::ConstraintManager context_t;

inline void concretize(context_t &ctx, klee::ref<klee::Expr> expr,
                       uint64_t value) {
  assert(expr->getWidth() <= 64);
  auto value_expr =
      kutil::solver_toolbox.exprBuilder->Constant(value, expr->getWidth());
  auto concretize_expr =
      kutil::solver_toolbox.exprBuilder->Eq(expr, value_expr);
  ctx.addConstraint(concretize_expr);
}

struct meta_t {
  std::unordered_map<node_id_t, uint64_t> hit_counter;
  uint64_t packet_counter;
  uint64_t accepted;
  uint64_t rejected;

  meta_t() : packet_counter(0) {}
};

struct rate_t {
  bool set;
  gbps_t value;
  rate_t() : set(false) {}
};

struct timeout_t {
  bool set;
  time_us_t value;
  timeout_t() : set(false) {}
};

struct cfg_t {
  rate_t rate;
  timeout_t timeout;
};

struct pkt_t {
  const uint8_t *data;
  uint32_t size;
};

typedef void (*operation_ptr)(const Call *call, pkt_t &pkt, time_ns_t time,
                              state_t &state, context_t &additional_ctx,
                              const cfg_t &cfg);

typedef std::map<std::string, operation_ptr> operations_t;

} // namespace emulation
} // namespace BDD