#pragma once

#include <string>
#include <vector>

#include "../../../synthesizer.h"
#include "klee-util.h"
#include "klee/Expr.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

class Variable {
protected:
  std::string label;
  unsigned size_bits;

  std::string prefix;
  std::pair<bool, std::string> vigor_symbol;
  std::vector<klee::ref<klee::Expr>> exprs;

public:
  Variable(const Variable &variable)
      : label(variable.label), size_bits(variable.size_bits),
        prefix(variable.prefix), vigor_symbol(variable.vigor_symbol),
        exprs(variable.exprs) {}

  Variable(std::string _label, unsigned _size_bits, std::string _vigor_symbol)
      : label(_label), size_bits(_size_bits),
        vigor_symbol(true, _vigor_symbol) {}

  Variable(std::string _label, unsigned _size_bits, klee::ref<klee::Expr> expr)
      : label(_label), size_bits(_size_bits), vigor_symbol(false, "") {
    exprs.push_back(expr);
  }

  Variable(std::string _label, unsigned _size_bits)
      : label(_label), size_bits(_size_bits), vigor_symbol(false, "") {}

  void set_prefix(const std::string &_prefix) { prefix = _prefix; }

  std::string get_label() const {
    if (prefix.size()) {
      return prefix + "." + label;
    }

    return label;
  }

  unsigned get_size_bits() const { return size_bits; }

  void add_expr(klee::ref<klee::Expr> expr) { exprs.push_back(expr); }

  void set_expr(klee::ref<klee::Expr> expr) {
    assert(exprs.size() == 0);
    return exprs.push_back(expr);
  }

  klee::ref<klee::Expr> get_expr() const {
    assert(exprs.size() == 1);
    return exprs[0];
  }

  virtual std::string get_type() const {
    std::stringstream type;

    type << "bit<";
    type << size_bits;
    type << ">";

    return type.str();
  }

  bool match(std::string s) const {
    if (!vigor_symbol.first) {
      return false;
    }

    return vigor_symbol.second.compare(s) == 0;
  }

  bool match(klee::ref<klee::Expr> e) const {
    for (auto expr : exprs) {
      auto eq = kutil::solver_toolbox.are_exprs_always_equal(expr, e);

      if (eq) {
        return true;
      }
    }

    return false;
  }

  kutil::solver_toolbox_t::contains_result_t
  contains(klee::ref<klee::Expr> e) const {
    for (auto expr : exprs) {
      auto contains_result = kutil::solver_toolbox.contains(expr, e);

      if (contains_result.contains) {
        return contains_result;
      }
    }

    return kutil::solver_toolbox_t::contains_result_t();
  }

  void synthesize(Synthesizer &synthesizer) const {
    auto type = get_type();

    synthesizer.indent();
    synthesizer.append(type);
    synthesizer.append(" ");
    synthesizer.append(label);
    synthesizer.append(";");
    synthesizer.append_new_line();
  }
};

struct variable_query_t {
  bool valid;
  std::unique_ptr<Variable> var;
  unsigned offset_bits;

  variable_query_t() : valid(false) {}

  variable_query_t(const Variable &_var, unsigned _offset_bits)
      : valid(true), var(std::unique_ptr<Variable>(new Variable(_var))),
        offset_bits(_offset_bits) {}
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse