#pragma once

#include "../../synthesizer.h"
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
  bool parsing_active;

public:
  IngressHeaders(int indentation)
      : synthesizer(indentation), parsing_active(true) {}

  void deactivate_parsing() { parsing_active = false; }
  bool is_parsing_active() const { return parsing_active; }
  void add(const header_t &hdr) { headers.push_back(hdr); }

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
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse