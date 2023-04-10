#pragma once

#include "klee/ExprBuilder.h"
#include "klee/util/ExprSMTLIBPrinter.h"
#include "klee/util/ExprVisitor.h"

#include "exprs.h"

#include <unordered_set>

namespace kutil {

klee::ref<klee::Expr> simplify(klee::ref<klee::Expr> expr);

} // namespace kutil