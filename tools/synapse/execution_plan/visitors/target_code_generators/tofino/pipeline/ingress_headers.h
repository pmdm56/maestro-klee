#pragma once

#include "../../constants.h"
#include "../../synthesizer.h"
#include "../constants.h"
#include "domain/header.h"

#include <assert.h>
#include <unordered_set>
#include <vector>

namespace synapse {
namespace synthesizer {
namespace tofino {

class IngressHeaders {
private:
  Synthesizer synthesizer;
  std::vector<header_t> headers;
  std::vector<Variable> fields;
  bool parsing_active;

public:
  IngressHeaders(int indentation)
      : synthesizer(indentation), parsing_active(true) {}

  void deactivate_parsing() { parsing_active = false; }
  bool is_parsing_active() const { return parsing_active; }

  void add(const header_t &hdr) {
    headers.push_back(hdr);

    auto offset = 0u;
    auto chunk = hdr.chunk;

    for (const auto &field : hdr.fields) {
      std::stringstream field_label;

      field_label << INGRESS_PACKET_HEADER_VARIABLE;
      field_label << ".";
      field_label << hdr.label;
      field_label << ".";
      field_label << field.get_label();

      auto label = field_label.str();
      auto size_bits = field.get_size_bits();

      auto field_expr =
          kutil::solver_toolbox.exprBuilder->Extract(chunk, offset, size_bits);

      fields.emplace_back(label, size_bits, field_expr);
      offset += size_bits;
    }
  }

  void synthesize(std::ostream &os) {
    std::unordered_set<std::string> defined_hdrs;

    for (auto header : headers) {
      if (defined_hdrs.find(header.type) != defined_hdrs.end()) {
        continue;
      }

      synthesizer.indent();
      synthesizer.append(header.type);
      synthesizer.append(" ");
      synthesizer.append(header.label);
      synthesizer.append(";");
      synthesizer.append_new_line();

      defined_hdrs.insert(header.type);
    }

    synthesizer.dump(os);
  }

  variable_query_t get_hdr_field_from_chunk(klee::ref<klee::Expr> chunk) const {
    auto symbol = kutil::get_symbol(chunk);

    if (!symbol.first || symbol.second != VIGOR_PACKET_CHUNK_SYMBOL) {
      return variable_query_t();
    }

    auto chunk_size_bits = chunk->getWidth();

    for (const auto &field : fields) {
      auto field_size_bits = field.get_size_bits();
      auto field_expr = field.get_expr();

      auto contains_result = kutil::solver_toolbox.contains(field_expr, chunk);

      if (contains_result.contains) {
        return variable_query_t(field, contains_result.offset_bits);
      }

      if (chunk_size_bits > field_size_bits) {
        continue;
      }

      for (auto byte = 0u; byte * 8 + chunk_size_bits <= field_size_bits;
           byte++) {

        auto field_expr_extracted = kutil::solver_toolbox.exprBuilder->Extract(
            field_expr, byte * 8, chunk_size_bits);

        if (kutil::solver_toolbox.are_exprs_always_equal(field_expr_extracted,
                                                         chunk)) {
          return variable_query_t(field, byte * 8);
        }
      }
    }

    return variable_query_t();
  }
}; // namespace tofino

} // namespace tofino
} // namespace synthesizer
} // namespace synapse