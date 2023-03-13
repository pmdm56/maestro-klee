#pragma once

#include "../../../../../symbex.h"
#include "../../code_builder.h"
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
  CodeBuilder defs;
  CodeBuilder decl;
  std::vector<Header> headers;

public:
  Headers(int ind_def, int ind_decl) : defs(ind_def), decl(ind_decl) {}

  void add(const Header &hdr) { headers.push_back(hdr); }

  void synthesize_defs(std::ostream &os) {
    auto unique_headers = get_unique_headers();

    for (auto header : unique_headers) {
      defs.indent();
      defs.append("header ");
      defs.append(header.get_type());
      defs.append(" {");
      defs.append_new_line();

      defs.inc_indentation();

      for (auto field : header.get_fields()) {
        defs.indent();
        defs.append(field.get_type());
        defs.append(" ");
        defs.append(field.get_label());
        defs.append(";");
        defs.append_new_line();
      }

      defs.dec_indentation();
      defs.append("}");
      defs.append_new_line();
      defs.append_new_line();
    }

    defs.dump(os);
  }

  void synthesize_decl(std::ostream &os) {
    auto unique_headers = get_unique_headers();

    for (auto header : unique_headers) {
      auto type = header.get_type();
      auto label = header.get_label();

      decl.indent();
      decl.append(type);
      decl.append(" ");
      decl.append(label);
      decl.append(";");
      decl.append_new_line();
    }

    decl.dump(os);
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