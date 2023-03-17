#include "transpiler.h"
#include "../../../../log.h"
#include "klee-util.h"
#include "x86_tofino_generator.h"

#include <assert.h>

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

class InternalTranspiler : public klee::ExprVisitor::ExprVisitor {
private:
  x86TofinoGenerator &tg;
  std::stringstream code;

public:
  InternalTranspiler(x86TofinoGenerator &_tg) : tg(_tg) {}

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

std::pair<bool, std::string>
try_transpile_constant(x86TofinoGenerator &tg, const klee::ref<klee::Expr> &expr) {
  auto result = std::pair<bool, std::string>();
  auto is_constant = kutil::is_constant(expr);

  if (is_constant) {
    auto value = kutil::solver_toolbox.value_from_expr(expr);
    result.first = true;
    result.second = std::to_string(value);
  }

  return result;
}

std::pair<bool, std::string>
try_transpile_variable(x86TofinoGenerator &tg, const klee::ref<klee::Expr> &expr) {
  assert(false && "TODO");
}

struct transpile_var_comparison_t {
  bool valid;
  std::string lhs;
  std::string rhs;

  transpile_var_comparison_t() : valid(false) {}

  transpile_var_comparison_t(const std::string &_lhs, const std::string &_rhs)
      : valid(true), lhs(_lhs), rhs(_rhs) {}
};

transpile_var_comparison_t
transpile_var_comparison(x86TofinoGenerator &tg, const klee::ref<klee::Expr> &lhs,
                         const klee::ref<klee::Expr> &rhs) {
  assert(false && "TODO");
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
  assert(false && "TODO");
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

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse