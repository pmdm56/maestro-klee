#pragma once

#include "klee/ExprBuilder.h"
#include "klee/util/ExprSMTLIBPrinter.h"
#include "klee/util/ExprVisitor.h"

#include "rename_symbols.h"

namespace kutil {

class SwapPacketEndianness : public klee::ExprVisitor::ExprVisitor {
private:
  std::map<klee::ref<klee::Expr>, klee::ref<klee::Expr>> replacements;
  std::string target_label;
  std::string temporary_label;

public:
  SwapPacketEndianness()
      : klee::ExprVisitor::ExprVisitor(true), target_label("packet_chunks"),
        temporary_label("packet_chunks_replaced") {}

  void clear_replacements() { replacements.clear(); }

  bool already_replaced(klee::ref<klee::Expr> expr) const {
    return replacements.find(expr) != replacements.end();
  }

  bool has_swapped() const { return replacements.size(); }

  klee::ref<klee::Expr> swap(klee::ref<klee::Expr> expr) {
    if (expr.isNull()) {
      return expr;
    }

    clear_replacements();

    RenameSymbols renamer;
    renamer.add_translation(target_label, temporary_label);

    auto renamed = renamer.rename(expr);
    auto swapped = visit(renamed);

    renamer.remove_translation(target_label);
    renamer.add_translation(temporary_label, target_label);
    auto renamed_back = renamer.rename(swapped);

    return renamed_back;
  }

  klee::ExprVisitor::Action visitExprPost(const klee::Expr &e) {
    auto eref = klee::ref<klee::Expr>(const_cast<klee::Expr *>(&e));
    auto it = replacements.find(eref);

    if (it != replacements.end()) {
      return Action::changeTo(it->second);
    }

    return Action::doChildren();
  }

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    auto ul = e.updates;
    auto index = e.index;
    auto root = ul.root;
    auto symbol = root->getName();

    if (symbol != temporary_label) {
      return Action::skipChildren();
    }

    auto replaced = klee::expr::ExprHandle(const_cast<klee::ReadExpr *>(&e));

    if (already_replaced(replaced)) {
      return Action::skipChildren();
    }

    assert(index->getKind() == klee::Expr::Constant);
    auto index_const = static_cast<klee::ConstantExpr *>(index.get());

    auto index_value = index_const->getZExtValue();
    auto new_index_value = index_value;

    switch (index_value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      new_index_value = 5 - index_value;
      break;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
      new_index_value = 17 - index_value;
      break;
    case 53:
    case 54:
    case 55:
    case 56:
      new_index_value = 109 - index_value;
      break;
    case 57:
    case 58:
    case 59:
    case 60:
      new_index_value = 117 - index_value;
      break;
    case 82:
    case 83:
      new_index_value = 165 - index_value;
      break;
    case 84:
    case 85:
      new_index_value = 169 - index_value;
      break;
    default:
      break;
    }

    if (new_index_value != index_value) {
      auto new_index = solver_toolbox.exprBuilder->Constant(new_index_value,
                                                            index->getWidth());
      auto replacement = solver_toolbox.exprBuilder->Read(ul, new_index);

      replacements.insert({replaced, replacement});

      return Action::changeTo(replacement);
    }

    return Action::skipChildren();
  }
};

} // namespace kutil