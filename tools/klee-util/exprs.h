#pragma once

#include "klee/ExprBuilder.h"
#include "klee/util/ExprSMTLIBPrinter.h"
#include "klee/util/ExprVisitor.h"

#include <unordered_set>

namespace kutil {

bool get_bytes_read(klee::ref<klee::Expr> expr, std::vector<unsigned> &bytes);
bool is_readLSB(klee::ref<klee::Expr> expr);
bool is_bool(klee::ref<klee::Expr> expr);
bool is_constant(klee::ref<klee::Expr> expr);
bool is_constant_signed(klee::ref<klee::Expr> expr);
int64_t get_constant_signed(klee::ref<klee::Expr> expr);

std::unordered_set<std::string> get_symbols(klee::ref<klee::Expr> expr);
std::pair<bool, std::string> get_symbol(klee::ref<klee::Expr> expr);

} // namespace kutil