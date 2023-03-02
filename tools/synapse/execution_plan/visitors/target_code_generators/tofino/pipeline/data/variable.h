#pragma once

#include <string>
#include <vector>

#include "klee/Expr.h"
#include "util.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

struct variable_t {
  std::string label;
  unsigned size_bits;

  std::pair<bool, std::string> vigor_symbol;
  std::vector<klee::ref<klee::Expr>> exprs;

  variable_t(std::string _label_prefix, std::string _label, unsigned _size_bits,
             std::string _vigor_symbol)
      : label(_label_prefix + "." + _label), size_bits(_size_bits),
        vigor_symbol(true, _vigor_symbol) {}

  variable_t(std::string _label_prefix, std::string _label, unsigned _size_bits)
      : variable_t(_label_prefix, _label, _size_bits, "") {}

  bool match(std::string s) const {
    if (!vigor_symbol.first) {
      return false;
    }

    return vigor_symbol.second.compare(s) == 0;
  }

  bool match(klee::ref<klee::Expr> e) const {
    for (auto expr : exprs) {
      auto eq = util::solver_toolbox.are_exprs_always_equal(expr, e);

      if (eq) {
        return true;
      }
    }

    return false;
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse