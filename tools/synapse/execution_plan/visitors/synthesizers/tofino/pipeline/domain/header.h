#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "klee/Expr.h"
#include "variable.h"

#include "../../../../../../symbex.h"
#include "../../constants.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

enum hdr_field_id_t {
  ETH_DST_ADDR,
  ETH_SRC_ADDR,
  ETH_ETHER_TYPE,
  IPV4_VERSION_IHL,
  IPV4_ECN_DSCP,
  IPV4_TOT_LEN,
  IPV4_ID,
  IPV4_FRAG_OFF,
  IPV4_TTL,
  IPV4_PROTOCOL,
  IPV4_CHECK,
  IPV4_SRC_IP,
  IPV4_DST_IP,
  IPV4_OPTIONS_VALUE,
  TCPUDP_SRC_PORT,
  TCPUDP_DST_PORT,
};

struct hdr_field_t : public Variable {
  hdr_field_id_t hdr_field_id;
  klee::ref<klee::Expr> var_length;

  hdr_field_t(hdr_field_id_t _hdr_field_id, const std::string &_label,
              unsigned _size_bits)
      : Variable(_label, _size_bits), hdr_field_id(_hdr_field_id) {}

  hdr_field_t(hdr_field_id_t _hdr_field_id, const std::string &_label,
              unsigned _size_bits, klee::ref<klee::Expr> _var_length)
      : Variable(_label, _size_bits), hdr_field_id(_hdr_field_id),
        var_length(_var_length) {}

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
};

enum hdr_id_t {
  ETHERNET,
  IPV4,
  IPV4_OPTIONS,
  TCPUDP,
};

class Header : public Variable {
private:
  hdr_id_t hdr_id;
  std::vector<hdr_field_t> fields;

public:
  Header(hdr_id_t _hdr_id, const std::string &_label,
         const klee::ref<klee::Expr> &_chunk,
         const std::vector<hdr_field_t> &_fields)
      : Variable(_label, _chunk->getWidth()), hdr_id(_hdr_id), fields(_fields) {
    assert(!_chunk.isNull());
    exprs.push_back(_chunk);

    bool size_check = true;
    unsigned total_size_bits = 0;
    auto offset = 0u;

    if (fields.size() == 1 && !fields[0].var_length.isNull()) {
      fields[0].set_expr(_chunk);
      return;
    }

    for (auto &field : fields) {
      auto field_size_bits = field.get_size_bits();

      total_size_bits += field_size_bits;
      size_check &= field.var_length.isNull();

      auto field_expr = kutil::solver_toolbox.exprBuilder->Extract(
          _chunk, offset, field_size_bits);

      field.set_expr(field_expr);
      offset += field_size_bits;
    }

    assert(!size_check || total_size_bits == _chunk->getWidth());
  }

public:
  hdr_id_t get_id() const { return hdr_id; }

  const std::vector<hdr_field_t> &query_fields() const { return fields; }

  variable_query_t query_field_var(hdr_field_id_t hdr_field_id) const {
    for (const auto &field : fields) {
      if (field.hdr_field_id != hdr_field_id) {
        continue;
      }

      std::stringstream field_label;

      field_label << INGRESS_PACKET_HEADER_VARIABLE;
      field_label << ".";
      field_label << label;
      field_label << ".";
      field_label << field.get_label();

      auto label = field_label.str();
      auto size_bits = field.get_size_bits();
      auto expr = field.get_expr();

      auto var = Variable(label, size_bits, expr);

      return variable_query_t(var, 0);
    }

    return variable_query_t();
  }

  variable_query_t query_field(klee::ref<klee::Expr> expr) const {
    auto symbol = kutil::get_symbol(expr);

    if (!symbol.first || symbol.second != symbex::CHUNK) {
      return variable_query_t();
    }

    for (const auto &field : fields) {
      auto field_expr = field.get_expr();
      auto contains_result = kutil::solver_toolbox.contains(field_expr, expr);

      if (contains_result.contains) {
        auto field_var = query_field_var(field.hdr_field_id);
        field_var.offset_bits = contains_result.offset_bits;
        return field_var;
      }
    }

    return variable_query_t();
  }

  std::string get_type() const override { return label + "_t"; }

  bool operator==(const Header &hdr) const {
    auto this_type = get_type();
    auto other_type = hdr.get_type();

    return hdr.label.compare(label) == 0 && this_type.compare(other_type) == 0;
  }

  struct hdr_hash_t {
    size_t operator()(const Header &hdr) const {
      auto type = hdr.get_type();
      auto hash = std::hash<std::string>()(hdr.label + type);
      return hash;
    }
  };
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse