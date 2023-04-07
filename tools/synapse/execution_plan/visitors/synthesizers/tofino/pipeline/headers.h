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
  std::vector<Header> headers;

public:
  Headers() {}

  void add(const Header &hdr) { headers.push_back(hdr); }

  variable_query_t
  query_hdr_field_from_chunk(klee::ref<klee::Expr> chunk) const {
    auto symbol = kutil::get_symbol(chunk);

    if (!symbol.first || symbol.second != symbex::CHUNK) {
      return variable_query_t();
    }

    for (const auto &hdr : headers) {
      auto varq = hdr.query_field(chunk);

      if (varq.valid) {
        return varq;
      }
    }

    return variable_query_t();
  }

  variable_query_t get_header(hdr_id_t hdr_id) const {
    for (const auto &hdr : headers) {
      if (hdr.get_id() == hdr_id) {
        return variable_query_t(hdr, 0);
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