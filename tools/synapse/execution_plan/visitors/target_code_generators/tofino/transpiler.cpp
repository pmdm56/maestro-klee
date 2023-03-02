#include "transpiler.h"
#include "../../../../log.h"

#include "util.h"

#include <assert.h>

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string transpile(const TofinoGenerator &tg,
                      const klee::ref<klee::Expr> &expr) {
  if (util::is_constant(expr)) {
    auto constant = static_cast<klee::ConstantExpr *>(expr.get());
    auto width = constant->getWidth();
    auto value = constant->getZExtValue();

    std::stringstream ss;

    if (width == 1) {
      ss << (value == 0 ? "false" : "true");
    } else {
      assert(constant->getWidth() <= 64);
      ss << width << "w" << value;
    }

    return ss.str();
  }

  Transpiler transpiler(tg);
  transpiler.visit(expr);
  auto code = transpiler.get();

  if (!code.size()) {
    Log::err() << "Unable to transpile expression: "
               << util::expr_to_string(expr, true) << "\n";
    Log::err() << "Kind: " << expr->getKind() << "\n";
    exit(1);
  }

  return code;
}

klee::ExprVisitor::Action Transpiler::visitRead(const klee::ReadExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSelect(const klee::SelectExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::ConcatExpr *>(&e);

  if (util::is_readLSB(eref)) {
    auto symbol = util::get_symbol(eref);
    auto variable = tg.search_variable(symbol);

    if (!variable) {
      Log::err() << "Unknown variable with symbol " << symbol << "\n";
      exit(1);
    }

    auto expr_width = eref->getWidth();
    auto var_width = variable->size_bits;

    if (expr_width != var_width) {
      auto max_width = std::max(expr_width, var_width);
      code << "(";
      code << "(bit<";
      code << max_width;
      code << ">)";
      code << variable->label;
      code << ")";
    } else {
      code << variable->label;
    }

    return klee::ExprVisitor::Action::skipChildren();
  }

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitExtract(const klee::ExtractExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitZExt(const klee::ZExtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSExt(const klee::SExtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitAdd(const klee::AddExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSub(const klee::SubExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitMul(const klee::MulExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitUDiv(const klee::UDivExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSDiv(const klee::SDivExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitURem(const klee::URemExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSRem(const klee::SRemExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitNot(const klee::NotExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitAnd(const klee::AndExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitOr(const klee::OrExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitXor(const klee::XorExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitShl(const klee::ShlExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitLShr(const klee::LShrExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitAShr(const klee::AShrExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitEq(const klee::EqExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_code = transpile(tg, lhs);
  auto rhs_code = transpile(tg, rhs);

  code << "(" << lhs_code << ")";
  code << " == ";
  code << "(" << rhs_code << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitNe(const klee::NeExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitUlt(const klee::UltExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitUle(const klee::UleExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitUgt(const klee::UgtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitUge(const klee::UgeExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSlt(const klee::SltExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSle(const klee::SleExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSgt(const klee::SgtExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSge(const klee::SgeExpr &) {
  assert(false && "TODO");
}

} // namespace tofino
} // namespace synthesizer
} // namespace synapse