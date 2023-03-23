#pragma once

#include "../../memory_bank.h"
#include "klee-util.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

class x86TofinoMemoryBank : public MemoryBank {
public:
  struct expiration_t {
    bool set;
    klee::ref<klee::Expr> chain;
    klee::ref<klee::Expr> map;
    klee::ref<klee::Expr> vector;
    klee::ref<klee::Expr> time;

    expiration_t() : set(false) {}
    expiration_t(klee::ref<klee::Expr> _chain, klee::ref<klee::Expr> _map,
                 klee::ref<klee::Expr> _vector, klee::ref<klee::Expr> _time)
        : set(true), chain(_chain), map(_map), vector(_vector), time(_time) {}
  };

  struct vector_borrow_t {
    klee::ref<klee::Expr> vector_addr;
    klee::ref<klee::Expr> value_out;
  };

  enum ds_type_t {
    MAP,
    DCHAIN,
  };

  struct ds_t {
    ds_type_t type;
    klee::ref<klee::Expr> addr;
    uint64_t node_id;

    ds_t(ds_type_t _type, klee::ref<klee::Expr> _addr, uint64_t _node_id)
        : type(_type), addr(_addr), node_id(_node_id) {}

    bool matches(ds_type_t _type, klee::ref<klee::Expr> _addr) const {
      if (type != _type) {
        return false;
      }

      return matches(_addr);
    }

    bool matches(klee::ref<klee::Expr> _addr) const {
      if (!kutil::solver_toolbox.are_exprs_always_equal(addr, _addr)) {
        return false;
      }

      return true;
    }
  };

  struct map_t : ds_t {
    bits_t value_size;

    map_t(klee::ref<klee::Expr> _addr, bits_t _value_size, uint64_t _node_id)
        : ds_t(ds_type_t::MAP, _addr, _node_id), value_size(_value_size) {}
  };

  struct dchain_t : ds_t {
    uint64_t index_range;

    dchain_t(klee::ref<klee::Expr> _addr, uint64_t _node_id,
             uint64_t _index_range)
        : ds_t(ds_type_t::DCHAIN, _addr, _node_id), index_range(_index_range) {}
  };

private:
  BDD::symbol_t time;
  expiration_t expiration;
  std::vector<vector_borrow_t> vector_borrows;
  std::vector<std::shared_ptr<ds_t>> data_structures;

public:
  x86TofinoMemoryBank() : MemoryBank() {}
  x86TofinoMemoryBank(const MemoryBank &mb) : MemoryBank(mb) {}
  x86TofinoMemoryBank(const x86TofinoMemoryBank &mb)
      : MemoryBank(mb), time(mb.time), expiration(mb.expiration),
        vector_borrows(mb.vector_borrows), data_structures(mb.data_structures) {
  }

  void set_time(BDD::symbol_t _time) { time = _time; }
  BDD::symbol_t get_time() const { return time; }

  void set_expiration(const expiration_t &_expiration) {
    expiration = _expiration;
  }

  expiration_t get_expiration() const { return expiration; }

  void set_vector_borrow(const vector_borrow_t &vector_borrow) {
    auto found_it = find_vector_borrow(vector_borrow.vector_addr);

    if (found_it != vector_borrows.end()) {
      found_it->value_out = vector_borrow.value_out;
    } else {
      vector_borrows.push_back(vector_borrow);
    }
  }

  klee::ref<klee::Expr>
  get_vector_borrow_value(klee::ref<klee::Expr> vector_addr) {
    auto found_it = find_vector_borrow(vector_addr);

    if (found_it == vector_borrows.end()) {
      return nullptr;
    }

    auto value = found_it->value_out;
    return value;
  }

  bool has_data_structure(klee::ref<klee::Expr> addr) const {
    for (auto ds : data_structures) {
      if (ds->matches(addr)) {
        return true;
      }
    }

    return false;
  }

  void add_data_structure(std::shared_ptr<ds_t> ds) {
    assert(!has_data_structure(ds->addr));
    data_structures.push_back(ds);
  }

  const std::vector<std::shared_ptr<ds_t>> &get_data_structures() const {
    return data_structures;
  }

  virtual MemoryBank_ptr clone() const {
    auto clone = new x86TofinoMemoryBank(*this);
    return MemoryBank_ptr(clone);
  }

private:
  std::vector<vector_borrow_t>::iterator
  find_vector_borrow(klee::ref<klee::Expr> vector_addr) {
    for (auto it = vector_borrows.begin(); it != vector_borrows.end(); it++) {
      auto eq = kutil::solver_toolbox.are_exprs_always_equal(it->vector_addr,
                                                             vector_addr);
      if (eq) {
        return it;
      }
    }

    return vector_borrows.end();
  }
};

} // namespace x86_tofino
} // namespace targets
} // namespace synapse