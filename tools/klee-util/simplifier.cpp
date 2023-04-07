#include "simplifier.h"
#include "solver_toolbox.h"

#include <assert.h>

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

} // namespace kutil