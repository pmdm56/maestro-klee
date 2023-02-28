#include "transpiler.h"
#include "../../../../log.h"

#include "util.h"

#include <assert.h>

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string Transpiler::transpile(const klee::ref<klee::Expr> &expr) {
  code.clear();
  visit(expr);

  auto output = code.str();

  if (!output.size()) {
    // error
    std::stringstream error;
    error << "Unable to transpile expression: ";
    error << util::expr_to_string(expr, true);
    error << "\n";
    error << "Kind: ";
    error << expr->getKind();
    error << "\n";

    Log::err() << error.str();
    exit(1);
  }

  return output;
}

klee::ExprVisitor::Action Transpiler::visitRead(const klee::ReadExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitSelect(const klee::SelectExpr &) {
  assert(false && "TODO");
}

klee::ExprVisitor::Action Transpiler::visitConcat(const klee::ConcatExpr &) {
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