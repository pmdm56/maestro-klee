#pragma once

#include <stack>

#include "../../synthesizer.h"
#include "domain/variable.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

constexpr char INGRESS_INTRINSIC_METADATA_VARIABLE[] = "ig_intr_md";
constexpr char INGRESS_USER_METADATA_VARIABLE[] = "meta";

class Ingress {
private:
  std::vector<variable_t> intrinsic_metadata;
  std::vector<variable_t> user_metadata;
  std::stack<bool> pending_ifs;

public:
  Synthesizer synthesizer;

  Ingress(int indentation) : synthesizer(indentation) {
    intrinsic_metadata = std::vector<variable_t>{
        {INGRESS_INTRINSIC_METADATA_VARIABLE, "resubmit_flag", 1},
        {INGRESS_INTRINSIC_METADATA_VARIABLE, "packet_version", 2},
        {INGRESS_INTRINSIC_METADATA_VARIABLE, "ingress_port", 9,
         "VIGOR_DEVICE"},
        {INGRESS_INTRINSIC_METADATA_VARIABLE, "ingress_mac_tstamp", 48},
    };
  }

  void push_pending_if() { pending_ifs.push(true); }
  void pop_pending_if() { pending_ifs.pop(); }

  // returns number of closed stacked blocks
  int close_pending_ifs() {
    int closed = 0;

    if (!pending_ifs.size()) {
      return closed;
    }

    while (pending_ifs.size()) {
      synthesizer.dec_indenation();
      synthesizer.indent();
      synthesizer.append("}");
      synthesizer.append_new_line();

      closed++;

      auto if_clause = pending_ifs.top();
      pending_ifs.pop();

      if (if_clause) {
        pending_ifs.push(false);
        break;
      }
    }

    return closed;
  }

  const variable_t *search_variable(std::string symbol) const {
    std::stringstream label;

    for (const auto &intrinsic_meta : intrinsic_metadata) {
      if (intrinsic_meta.match(symbol)) {
        return &intrinsic_meta;
      }
    }

    for (const auto &user_meta : user_metadata) {
      if (user_meta.match(symbol)) {
        return &user_meta;
      }
    }

    return nullptr;
  }

  void synthesize(std::ostream& os) {
    synthesizer.dump(os);
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse