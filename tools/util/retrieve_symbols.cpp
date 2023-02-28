#include "retrieve_symbols.h"

namespace util {

bool get_bytes_read(klee::ref<klee::Expr> expr, std::vector<unsigned> &bytes) {
  switch (expr->getKind()) {
  case klee::Expr::Kind::Read: {
    klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(expr);
    auto index = read->index;

    if (index->getKind() != klee::Expr::Kind::Constant) {
      return false;
    }

    klee::ConstantExpr *index_const =
        static_cast<klee::ConstantExpr *>(index.get());
    bytes.push_back(index_const->getZExtValue());

    return true;
  };
  case klee::Expr::Kind::Concat: {
    klee::ConcatExpr *concat = dyn_cast<klee::ConcatExpr>(expr);

    auto left = concat->getLeft();
    auto right = concat->getRight();

    if (!get_bytes_read(left, bytes) || !get_bytes_read(right, bytes)) {
      return false;
    }

    return true;
  };
  default: {
  }
  }

  return false;
}

bool is_readLSB_complete(klee::ref<klee::Expr> expr) {
  auto size = expr->getWidth();
  assert(size % 8 == 0);
  size /= 8;

  RetrieveSymbols retriever;
  retriever.visit(expr);

  if (retriever.get_retrieved_strings().size() > 1) {
    return false;
  }

  std::vector<unsigned> bytes_read;
  if (!get_bytes_read(expr, bytes_read)) {
    return false;
  }

  unsigned expected_byte = size - 1;
  for (const auto &byte : bytes_read) {
    if (byte != expected_byte) {
      return false;
    }

    expected_byte -= 1;
  }

  return true;
}

} // namespace util