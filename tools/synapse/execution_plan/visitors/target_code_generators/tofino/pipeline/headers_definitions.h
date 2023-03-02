#pragma once

#include "../../synthesizer.h"
#include "domain/header.h"

#include <assert.h>
#include <unordered_set>
#include <vector>

namespace synapse {
namespace synthesizer {
namespace tofino {

class HeadersDefinitions {
private:
  Synthesizer synthesizer;
  std::unordered_set<header_t, header_t::header_hash_t> headers;

public:
  HeadersDefinitions(int indentation) : synthesizer(indentation) {}

  void add(const header_t &hdr) { headers.insert(hdr); }

  void synthesize(std::ostream &os) {
    for (auto header : headers) {
      synthesizer.indent();
      synthesizer.append("header ");
      synthesizer.append(header.type);
      synthesizer.append(" {");
      synthesizer.append_new_line();

      synthesizer.inc_indenation();

      for (auto field : header.fields) {
        synthesizer.indent();
        synthesizer.append(field.type);
        synthesizer.append(" ");
        synthesizer.append(field.label);
        synthesizer.append(";");
        synthesizer.append_new_line();
      }

      synthesizer.dec_indenation();
      synthesizer.append("}");
      synthesizer.append_new_line();
      synthesizer.append_new_line();
    }

    synthesizer.dump(os);
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse