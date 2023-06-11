#pragma once

#include "call-paths-to-bdd.h"
#include "klee-util.h"

#include <unordered_map>
#include <vector>

#include "data_structures/data_structures.h"
#include "internals.h"
#include "operations/operations.h"

namespace BDD {
namespace emulation {

class Emulator {
private:
  const BDD &bdd;
  cfg_t cfg;

  state_t state;
  meta_t meta;

  operations_t operations;

public:
  Emulator(const BDD &_bdd, cfg_t _cfg)
      : bdd(_bdd), cfg(_cfg), operations(get_operations()) {
    kutil::solver_toolbox.build();
    setup();
  }

  void list_operations() const;

  void run(pkt_t pkt, time_ns_t time, uint16_t device);
  void run(const std::string &pcap_file, uint16_t device);

  std::unordered_map<node_id_t, hit_rate_t> get_hit_rate() const;

private:
  void dump_context(const context_t &additional_ctx) const;
  bool evaluate_condition(klee::ref<klee::Expr> condition,
                          context_t &additional_ctx) const;
  operation_ptr get_operation(const std::string &name) const;
  void process(Node_ptr node, pkt_t pkt, time_ns_t time,
               context_t &additional_ctx);
  void setup();
};

} // namespace emulation
} // namespace BDD