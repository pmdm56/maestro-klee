#include "transpiler.h"
#include "../../../../log.h"
#include "klee-util.h"
#include "tofino_generator.h"
#include "util.h"

#include <assert.h>

namespace synapse {
namespace synthesizer {
namespace tofino {

class InternalTranspiler : public klee::ExprVisitor::ExprVisitor {
private:
  TofinoGenerator &tg;
  std::stringstream code;

public:
  InternalTranspiler(TofinoGenerator &_tg) : tg(_tg) {}

  std::string get() const { return code.str(); }

private:
  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &);
  klee::ExprVisitor::Action visitSelect(const klee::SelectExpr &);
  klee::ExprVisitor::Action visitConcat(const klee::ConcatExpr &);
  klee::ExprVisitor::Action visitExtract(const klee::ExtractExpr &);
  klee::ExprVisitor::Action visitZExt(const klee::ZExtExpr &);
  klee::ExprVisitor::Action visitSExt(const klee::SExtExpr &);
  klee::ExprVisitor::Action visitAdd(const klee::AddExpr &);
  klee::ExprVisitor::Action visitSub(const klee::SubExpr &);
  klee::ExprVisitor::Action visitMul(const klee::MulExpr &);
  klee::ExprVisitor::Action visitUDiv(const klee::UDivExpr &);
  klee::ExprVisitor::Action visitSDiv(const klee::SDivExpr &);
  klee::ExprVisitor::Action visitURem(const klee::URemExpr &);
  klee::ExprVisitor::Action visitSRem(const klee::SRemExpr &);
  klee::ExprVisitor::Action visitNot(const klee::NotExpr &);
  klee::ExprVisitor::Action visitAnd(const klee::AndExpr &);
  klee::ExprVisitor::Action visitOr(const klee::OrExpr &);
  klee::ExprVisitor::Action visitXor(const klee::XorExpr &);
  klee::ExprVisitor::Action visitShl(const klee::ShlExpr &);
  klee::ExprVisitor::Action visitLShr(const klee::LShrExpr &);
  klee::ExprVisitor::Action visitAShr(const klee::AShrExpr &);
  klee::ExprVisitor::Action visitEq(const klee::EqExpr &);
  klee::ExprVisitor::Action visitNe(const klee::NeExpr &);
  klee::ExprVisitor::Action visitUlt(const klee::UltExpr &);
  klee::ExprVisitor::Action visitUle(const klee::UleExpr &);
  klee::ExprVisitor::Action visitUgt(const klee::UgtExpr &);
  klee::ExprVisitor::Action visitUge(const klee::UgeExpr &);
  klee::ExprVisitor::Action visitSlt(const klee::SltExpr &);
  klee::ExprVisitor::Action visitSle(const klee::SleExpr &);
  klee::ExprVisitor::Action visitSgt(const klee::SgtExpr &);
  klee::ExprVisitor::Action visitSge(const klee::SgeExpr &);
};

std::vector<std::string>
Transpiler::assign_key_bytes(klee::ref<klee::Expr> expr) {
  std::vector<std::string> assignments;
  auto size_bits = expr->getWidth();

  for (auto byte = 0u; byte * 8 < size_bits; byte++) {
    auto key_byte =
        kutil::solver_toolbox.exprBuilder->Extract(expr, byte * 8, 8);

    auto key_byte_code = tg.transpile(key_byte);
    auto key_byte_var = tg.allocate_key_byte(byte);

    std::stringstream assignment;

    assignment << key_byte_var.get_label();
    assignment << " = ";
    assignment << key_byte_code;

    assignments.push_back(assignment.str());
  }

  return assignments;
}

std::pair<bool, std::string>
try_transpile_constant(TofinoGenerator &tg, const klee::ref<klee::Expr> &expr) {
  auto result = std::pair<bool, std::string>();
  auto is_constant = kutil::is_constant(expr);

  if (is_constant) {
    auto constant = static_cast<klee::ConstantExpr *>(expr.get());
    auto width = constant->getWidth();
    auto value = constant->getZExtValue();

    auto ss = std::stringstream();

    if (width == 1) {
      ss << (value == 0 ? "false" : "true");
    } else {
      assert(constant->getWidth() <= 64);
      ss << width << "w" << value;
    }

    result.first = true;
    result.second = ss.str();

    return result;
  }

  auto value = kutil::solver_toolbox.value_from_expr(expr);
  auto const_expr =
      kutil::solver_toolbox.exprBuilder->Constant(value, expr->getWidth());
  auto eq_const = kutil::solver_toolbox.exprBuilder->Eq(const_expr, expr);

  auto always_eq_const = kutil::solver_toolbox.is_expr_always_true(eq_const);

  if (always_eq_const) {
    result.first = true;
    result.second = std::to_string(value);
  }

  return result;
}

std::pair<bool, std::string>
try_transpile_variable(TofinoGenerator &tg, const klee::ref<klee::Expr> &expr) {
  auto result = std::pair<bool, std::string>();
  auto variable = tg.search_variable(expr);

  if (!variable.valid) {
    return result;
  }

  std::stringstream transpilation_builder;
  transpilation_builder << variable.var->get_label();

  auto size_bits = expr->getWidth();

  if (variable.offset_bits > 0 || variable.var->get_size_bits() != size_bits) {
    auto lo = variable.offset_bits;
    auto hi = variable.offset_bits + expr->getWidth() - 1;

    transpilation_builder << "[";
    transpilation_builder << hi;
    transpilation_builder << ":";
    transpilation_builder << lo;
    transpilation_builder << "]";
  }

  result.first = true;
  result.second = transpilation_builder.str();

  return result;
}

struct transpile_bool_comparison_t {
  bool valid;
  std::string lhs;
  std::string rhs;

  transpile_bool_comparison_t() : valid(false) {}

  transpile_bool_comparison_t(const std::string &_lhs, const std::string &_rhs)
      : valid(true), lhs(_lhs), rhs(_rhs) {}
};

transpile_bool_comparison_t
transpile_bool_comparison(TofinoGenerator &tg, const klee::ref<klee::Expr> &lhs,
                          const klee::ref<klee::Expr> &rhs) {
  auto lhs_varq = tg.search_variable(lhs);
  auto rhs_varq = tg.search_variable(rhs);

  std::cerr << kutil::expr_to_string(rhs, true) << "\n";
  std::cerr << "rhs " << rhs_varq.valid << "\n";

  // Not interested if both are variables or both are not variables.
  if (!(lhs_varq.valid ^ rhs_varq.valid)) {
    return transpile_bool_comparison_t();
  }

  const auto &varq = lhs_varq.valid ? lhs_varq : rhs_varq;
  const auto &not_var_expr = rhs_varq.valid ? lhs : rhs;

  std::cerr << "valid " << varq.valid << " offset " << varq.offset_bits
            << " size " << varq.var->get_size_bits() << " label "
            << varq.var->get_label() << "\n";

  if (varq.offset_bits != 0 || varq.var->get_size_bits() != 1) {
    return transpile_bool_comparison_t();
  }

  auto zero =
      kutil::solver_toolbox.exprBuilder->Constant(0, not_var_expr->getWidth());
  auto eq_zero =
      kutil::solver_toolbox.are_exprs_always_equal(zero, not_var_expr);

  if (eq_zero) {
    return transpile_bool_comparison_t(varq.var->get_label(), "false");
  }

  return transpile_bool_comparison_t(varq.var->get_label(), "true");
}

std::string Transpiler::transpile(const klee::ref<klee::Expr> &expr) {
  auto const_result = try_transpile_constant(tg, expr);

  if (const_result.first) {
    return const_result.second;
  }

  auto variable_result = try_transpile_variable(tg, expr);

  if (variable_result.first) {
    return variable_result.second;
  }

  auto transpiler = InternalTranspiler(tg);
  transpiler.visit(expr);

  auto code = transpiler.get();

  if (!code.size()) {
    Log::err() << "Unable to transpile expression: "
               << kutil::expr_to_string(expr, true) << "\n";
    Log::err() << "Kind: " << expr->getKind() << "\n";
    exit(1);
  }

  return code;
}

klee::ExprVisitor::Action
InternalTranspiler::visitRead(const klee::ReadExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitSelect(const klee::SelectExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::ConcatExpr *>(&e);

  if (kutil::is_readLSB(eref)) {
    auto symbol = kutil::get_symbol(eref);
    auto variable = tg.search_variable(symbol.second);

    if (!variable.valid) {
      Log::err() << "Unknown variable with symbol " << symbol.second << "\n";
      exit(1);
    }

    assert(variable.offset_bits == 0);

    auto expr_width = eref->getWidth();
    auto var_width = variable.var->get_size_bits();

    if (expr_width != var_width) {
      auto max_width = std::max(expr_width, var_width);
      code << "(";
      code << "(bit<";
      code << max_width;
      code << ">)";
      code << variable.var->get_label();
      code << ")";
    } else {
      code << variable.var->get_label();
    }

    return klee::ExprVisitor::Action::skipChildren();
  }

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitExtract(const klee::ExtractExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitZExt(const klee::ZExtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitSExt(const klee::SExtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitAdd(const klee::AddExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSub(const klee::SubExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitMul(const klee::MulExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitUDiv(const klee::UDivExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitSDiv(const klee::SDivExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitURem(const klee::URemExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitSRem(const klee::SRemExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitNot(const klee::NotExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitAnd(const klee::AndExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitOr(const klee::OrExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitXor(const klee::XorExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitShl(const klee::ShlExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitLShr(const klee::LShrExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitAShr(const klee::AShrExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitEq(const klee::EqExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto transpile_bool_comparison_result =
      transpile_bool_comparison(tg, lhs, rhs);

  if (transpile_bool_comparison_result.valid) {
    code << transpile_bool_comparison_result.lhs;
    code << " == ";
    code << transpile_bool_comparison_result.rhs;

    return klee::ExprVisitor::Action::skipChildren();
  }

  auto lhs_code = tg.transpile(lhs);
  auto rhs_code = tg.transpile(rhs);

  code << "(" << lhs_code << ")";
  code << " == ";
  code << "(" << rhs_code << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitNe(const klee::NeExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitUlt(const klee::UltExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitUle(const klee::UleExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitUgt(const klee::UgtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitUge(const klee::UgeExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSlt(const klee::SltExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSle(const klee::SleExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSgt(const klee::SgtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSge(const klee::SgeExpr &) {
  assert(false && "TODO");
}

} // namespace tofino
} // namespace synthesizer
} // namespace synapse