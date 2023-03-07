#pragma once

#include "../../../../../symbex.h"
#include "../../synthesizer.h"
#include "../constants.h"
#include "domain/header.h"

#include <assert.h>
#include <unordered_set>
#include <vector>

namespace synapse {
namespace synthesizer {
namespace tofino {

class Headers {
private:
  Synthesizer synthesizer_defs;
  Synthesizer synthesizer_decl;
  std::vector<Header> headers;

public:
  Headers(int ind_def, int ind_decl)
      : synthesizer_defs(ind_def), synthesizer_decl(ind_decl) {}

  void add(const Header &hdr) { headers.push_back(hdr); }

  void synthesize_defs(std::ostream &os) {
    auto unique_headers = get_unique_headers();

    for (auto header : unique_headers) {
      synthesizer_defs.indent();
      synthesizer_defs.append("header ");
      synthesizer_defs.append(header.get_type());
      synthesizer_defs.append(" {");
      synthesizer_defs.append_new_line();

      synthesizer_defs.inc_indentation();

      for (auto field : header.get_fields()) {
        synthesizer_defs.indent();
        synthesizer_defs.append(field.get_type());
        synthesizer_defs.append(" ");
        synthesizer_defs.append(field.get_label());
        synthesizer_defs.append(";");
        synthesizer_defs.append_new_line();
      }

      synthesizer_defs.dec_indentation();
      synthesizer_defs.append("}");
      synthesizer_defs.append_new_line();
      synthesizer_defs.append_new_line();
    }

    synthesizer_defs.dump(os);
  }

  void synthesize_decl(std::ostream &os) {
    auto unique_headers = get_unique_headers();

    for (auto header : unique_headers) {
      auto type = header.get_type();
      auto label = header.get_label();

      synthesizer_decl.indent();
      synthesizer_decl.append(type);
      synthesizer_decl.append(" ");
      synthesizer_decl.append(label);
      synthesizer_decl.append(";");
      synthesizer_decl.append_new_line();
    }

    synthesizer_decl.dump(os);
  }

  variable_query_t get_hdr_field_from_chunk(klee::ref<klee::Expr> chunk) const {
    auto symbol = kutil::get_symbol(chunk);

    if (!symbol.first || symbol.second != symbex::CHUNK) {
      return variable_query_t();
    }

    for (const auto &hdr : headers) {
      auto varq = hdr.get_field(chunk);

      if (varq.valid) {
        return varq;
      }
    }

    return variable_query_t();
  }

  // variable_query_t get_hdr_field_var(hdr_id_t hdr_id,
  //                                    hdr_field_id_t hdr_field_id,
  //                                    int offset_bits) const {
  //   for (const auto &hdr : headers) {
  //     if (hdr.get_id() != hdr_id) {
  //       continue;
  //     }

  //     for (const auto &field : hdr.fields) {
  //       if (field.hdr_field_id != hdr_field_id) {
  //         continue;
  //       }

  //       auto var = build_hdr_field_var(hdr, field);

  //       return variable_query_t(var, offset_bits);
  //     }
  //   }

  //   return variable_query_t();
  // }

private:
  std::unordered_set<Header, Header::hdr_hash_t> get_unique_headers() const {
    std::unordered_set<Header, Header::hdr_hash_t> unique_headers;

    for (const auto &hdr : headers) {
      unique_headers.insert(hdr);
    }

    return unique_headers;
  }
}; // namespace tofino

} // namespace tofino
} // namespace synthesizer
} // namespace synapse