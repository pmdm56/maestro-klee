#pragma once

#include <string>
#include <vector>

#include "klee/Expr.h"
#include "variable.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

struct header_field_t : public Variable {
  klee::ref<klee::Expr> var_length;

  header_field_t(const std::string &_label, unsigned _size_bits)
      : Variable(_label, _size_bits) {}

  header_field_t(const std::string &_label, unsigned _size_bits,
                 klee::ref<klee::Expr> _var_length)
      : Variable(_label, _size_bits), var_length(_var_length) {}

  std::string get_type() const override {
    std::stringstream type;

    if (!var_length.isNull()) {
      type << "varbit<";
      type << size_bits;
      type << ">";

      return type.str();
    }

    return Variable::get_type();
  }
}; // namespace tofino

struct header_t {
  std::string label;
  std::string type;
  klee::ref<klee::Expr> chunk;
  std::vector<header_field_t> fields;

  header_t(const std::string &_label, const klee::ref<klee::Expr> &_chunk,
           const std::vector<header_field_t> &_fields)
      : label(_label), type(_label + "_t"), chunk(_chunk), fields(_fields) {
    assert(!chunk.isNull());

    bool size_check = true;
    unsigned total_size_bits = 0;

    for (auto field : fields) {
      total_size_bits += field.get_size_bits();
      size_check &= field.var_length.isNull();
    }

    assert(!size_check || total_size_bits <= chunk->getWidth());
  }

  bool operator==(const header_t &hdr) const {
    return hdr.label.compare(label) == 0 && hdr.type.compare(type) == 0;
  }

  struct header_hash_t {
    size_t operator()(const header_t &hdr) const {
      auto hash = std::hash<std::string>()(hdr.label + hdr.type);
      return hash;
    }
  };
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse