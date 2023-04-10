#include "simplifier.h"
#include "printer.h"
#include "solver_toolbox.h"

#include <assert.h>
#include <unordered_map>

#define SIMPLIFY_CHILDREN_UNARY_OP(EXPR, OP)                                   \
  {                                                                            \
    auto kid = (EXPR)->getKid(0);                                              \
    kid = simplify(kid);                                                       \
    simplified = solver_toolbox.exprBuilder->OP(kid);                          \
  }

#define SIMPLIFY_CHILDREN_CAST_OP(EXPR, OP)                                    \
  {                                                                            \
    auto kid = (EXPR)->getKid(0);                                              \
    kid = simplify(kid);                                                       \
    auto width = (EXPR)->getWidth();                                           \
    simplified = solver_toolbox.exprBuilder->OP(kid, width);                   \
  }

#define SIMPLIFY_CHILDREN_BINARY_OP(EXPR, OP)                                  \
  {                                                                            \
    auto lhs = (EXPR)->getKid(0);                                              \
    auto rhs = (EXPR)->getKid(1);                                              \
    lhs = simplify(lhs);                                                       \
    rhs = simplify(rhs);                                                       \
    simplified = solver_toolbox.exprBuilder->OP(lhs, rhs);                     \
  }

#define SIMPLIFY_CHILDREN_TERNARY_OP(EXPR, OP)                                 \
  {                                                                            \
    auto k0 = (EXPR)->getKid(0);                                               \
    auto k1 = (EXPR)->getKid(1);                                               \
    auto k2 = (EXPR)->getKid(2);                                               \
    k0 = simplify(k0);                                                         \
    k1 = simplify(k1);                                                         \
    k2 = simplify(k2);                                                         \
    simplified = solver_toolbox.exprBuilder->OP(k0, k1, k2);                   \
  }

namespace kutil {

klee::ref<klee::Expr> simplify_extract(klee::ref<klee::Expr> extract_expr) {
  if (extract_expr->getKind() != klee::Expr::Extract) {
    return extract_expr;
  }

  auto extract = static_cast<klee::ExtractExpr *>(extract_expr.get());

  auto offset = extract->offset;
  auto expr = extract->expr;
  auto expr_size = expr->getWidth();
  auto size = extract->width;

  // concats
  while (expr->getKind() == klee::Expr::Kind::Concat) {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);

    auto lhs_size = lhs->getWidth();
    auto rhs_size = rhs->getWidth();

    // Either completely on the right side, or completely on the left side
    assert(rhs_size >= offset + size ||
           (offset >= rhs_size && lhs_size >= (offset - rhs_size) + size));

    if (rhs_size >= offset + size) {
      expr = rhs;
    } else {
      offset -= rhs_size;
      expr = lhs;
    }
  }

  assert(!expr.isNull());
  assert(size <= expr->getWidth());

  if (offset == 0 && size == expr->getWidth()) {
    return expr;
  }

  auto new_extract = solver_toolbox.exprBuilder->Extract(expr, offset, size);

  return new_extract;
}

bool is_eq_0(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &not_const_kid) {
  assert(!expr.isNull());

  if (expr->getKind() != klee::Expr::Eq) {
    return false;
  }

  auto lhs = expr->getKid(0);
  auto rhs = expr->getKid(1);

  auto lhs_is_constant = kutil::is_constant(lhs);
  auto rhs_is_constant = kutil::is_constant(rhs);

  if (!lhs_is_constant && !rhs_is_constant) {
    return false;
  }

  auto constant = lhs_is_constant ? lhs : rhs;
  auto other = lhs_is_constant ? rhs : lhs;
  auto constant_value = kutil::solver_toolbox.value_from_expr(constant);

  not_const_kid = other;

  return constant_value == 0;
}

klee::ref<klee::Expr> negate(klee::ref<klee::Expr> expr) {
  klee::ref<klee::Expr> other;

  if (is_eq_0(expr, other) && other->getWidth() == 1) {
    return other;
  }

  auto negate_map = std::unordered_map<klee::Expr::Kind, klee::Expr::Kind>{
      {klee::Expr::Or, klee::Expr::And},
      {klee::Expr::And, klee::Expr::Or},
      {klee::Expr::Eq, klee::Expr::Ne},
      {klee::Expr::Ne, klee::Expr::Eq},
      {klee::Expr::Ult, klee::Expr::Uge},
      {klee::Expr::Uge, klee::Expr::Ult},
      {klee::Expr::Ule, klee::Expr::Ugt},
      {klee::Expr::Ugt, klee::Expr::Ule},
      {klee::Expr::Slt, klee::Expr::Sge},
      {klee::Expr::Sge, klee::Expr::Slt},
      {klee::Expr::Sle, klee::Expr::Sgt},
      {klee::Expr::Sgt, klee::Expr::Sle},
      {klee::Expr::ZExt, klee::Expr::ZExt},
      {klee::Expr::SExt, klee::Expr::SExt},
  };

  auto kind = expr->getKind();
  auto found_it = negate_map.find(kind);
  assert(found_it != negate_map.end());

  switch (found_it->second) {
  case klee::Expr::Or: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    lhs = negate(lhs);
    rhs = negate(rhs);
    return kutil::solver_toolbox.exprBuilder->Or(lhs, rhs);
  } break;
  case klee::Expr::And: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    lhs = negate(lhs);
    rhs = negate(rhs);
    return kutil::solver_toolbox.exprBuilder->And(lhs, rhs);
  } break;
  case klee::Expr::Eq: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Eq(lhs, rhs);
  } break;
  case klee::Expr::Ne: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Ne(lhs, rhs);
  } break;
  case klee::Expr::Ult: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Ult(lhs, rhs);
  } break;
  case klee::Expr::Ule: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Ule(lhs, rhs);
  } break;
  case klee::Expr::Ugt: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Ugt(lhs, rhs);
  } break;
  case klee::Expr::Uge: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Uge(lhs, rhs);
  } break;
  case klee::Expr::Slt: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Slt(lhs, rhs);
  } break;
  case klee::Expr::Sle: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Sle(lhs, rhs);
  } break;
  case klee::Expr::Sgt: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Sgt(lhs, rhs);
  } break;
  case klee::Expr::Sge: {
    auto lhs = expr->getKid(0);
    auto rhs = expr->getKid(1);
    return kutil::solver_toolbox.exprBuilder->Sge(lhs, rhs);
  } break;
  case klee::Expr::ZExt: {
    auto kid = expr->getKid(0);
    auto width = expr->getWidth();
    kid = negate(kid);
    return kutil::solver_toolbox.exprBuilder->ZExt(kid, width);
  } break;
  case klee::Expr::SExt: {
    auto kid = expr->getKid(0);
    auto width = expr->getWidth();
    kid = negate(kid);
    return kutil::solver_toolbox.exprBuilder->SExt(kid, width);
  } break;
  default:
    assert(false && "TODO");
  }

  return expr;
}

bool simplify_cmp_eq_0(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  klee::ref<klee::Expr> other;

  if (!is_eq_0(expr, other)) {
    return false;
  }

  auto allowed_exprs = std::vector<klee::Expr::Kind>{
      klee::Expr::Or,  klee::Expr::And, klee::Expr::Eq,  klee::Expr::Ne,
      klee::Expr::Ult, klee::Expr::Ule, klee::Expr::Ugt, klee::Expr::Uge,
      klee::Expr::Slt, klee::Expr::Sle, klee::Expr::Sgt, klee::Expr::Sge,
  };

  auto found_it =
      std::find(allowed_exprs.begin(), allowed_exprs.end(), other->getKind());

  if (found_it == allowed_exprs.end()) {
    return false;
  }

  out = negate(other);
  return true;
}

class Simplifier : public klee::ExprVisitor::ExprVisitor {
private:
  klee::ref<klee::Expr> simplified;

public:
  Simplifier() {}

  klee::ref<klee::Expr> get() const { return simplified; }

private:
  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);
    simplified = expr;
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSelect(const klee::SelectExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SelectExpr *>(&e);
    SIMPLIFY_CHILDREN_TERNARY_OP(expr, Select);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitConcat(const klee::ConcatExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Concat);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitExtract(const klee::ExtractExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::ExtractExpr *>(&e);
    simplified = simplify_extract(expr);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitZExt(const klee::ZExtExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::ZExtExpr *>(&e);
    SIMPLIFY_CHILDREN_CAST_OP(expr, ZExt);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSExt(const klee::SExtExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SExtExpr *>(&e);
    SIMPLIFY_CHILDREN_CAST_OP(expr, SExt);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitAdd(const klee::AddExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::AddExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Add);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSub(const klee::SubExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SubExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Sub);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitMul(const klee::MulExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::MulExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Mul);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitUDiv(const klee::UDivExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::UDivExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, UDiv);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSDiv(const klee::SDivExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SDivExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, SDiv);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitURem(const klee::URemExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::URemExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, URem);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSRem(const klee::SRemExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SRemExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, SRem);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitNot(const klee::NotExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::NotExpr *>(&e);
    SIMPLIFY_CHILDREN_UNARY_OP(expr, Not);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitAnd(const klee::AndExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::AndExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, And);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitOr(const klee::OrExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::OrExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Or);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitXor(const klee::XorExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::XorExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Xor);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitShl(const klee::ShlExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::ShlExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Shl);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitLShr(const klee::LShrExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::LShrExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, LShr);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitAShr(const klee::AShrExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::AShrExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, AShr);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitEq(const klee::EqExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::EqExpr *>(&e);

    if (simplify_cmp_eq_0(expr, simplified)) {
      return klee::ExprVisitor::Action::skipChildren();
    }

    SIMPLIFY_CHILDREN_BINARY_OP(expr, Eq);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitNe(const klee::NeExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::NeExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Ne);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitUlt(const klee::UltExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::UltExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Ult);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitUle(const klee::UleExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::UleExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Ule);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitUgt(const klee::UgtExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::UgtExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Ugt);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitUge(const klee::UgeExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::UgeExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Uge);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSlt(const klee::SltExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SltExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Slt);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSle(const klee::SleExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SleExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Sle);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSgt(const klee::SgtExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SgtExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Sgt);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ExprVisitor::Action visitSge(const klee::SgeExpr &e) {
    klee::ref<klee::Expr> expr = const_cast<klee::SgeExpr *>(&e);
    SIMPLIFY_CHILDREN_BINARY_OP(expr, Sge);
    return klee::ExprVisitor::Action::skipChildren();
  }
};

klee::ref<klee::Expr> simplify(klee::ref<klee::Expr> expr) {
  if (expr->getKind() == klee::Expr::Constant) {
    return expr;
  }

  Simplifier simplifier;
  simplifier.visit(expr);

  return simplifier.get();
}

} // namespace kutil