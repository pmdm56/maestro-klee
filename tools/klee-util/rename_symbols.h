#pragma once

#include "klee/ExprBuilder.h"
#include "klee/Constraints.h"
#include "expr/Parser.h"
#include "klee/util/ExprVisitor.h"

#include "solver_toolbox.h"

namespace kutil {

class RenameSymbols : public klee::ExprVisitor::ExprVisitor {
private:
  std::map<std::string, std::string> translations;
  std::map<klee::ref<klee::Expr>, klee::ref<klee::Expr>> replacements;

public:
  RenameSymbols() {}
  RenameSymbols(const RenameSymbols &renamer)
      : klee::ExprVisitor::ExprVisitor(true),
        translations(renamer.translations), replacements(renamer.replacements) {
  }

  const std::map<std::string, std::string> &get_translations() const {
    return translations;
  }

  void add_translation(std::string before, std::string after) {
    translations[before] = after;
  }

  void remove_translation(std::string before) { translations.erase(before); }

  bool has_translation(std::string before) const {
    auto found_it = translations.find(before);
    return found_it != translations.end();
  }

  void clear_replacements() { replacements.clear(); }

  klee::ref<klee::Expr> rename(klee::ref<klee::Expr> expr) {
    if (expr.isNull()) {
      return expr;
    }

    clear_replacements();

    return visit(expr);
  }

  std::vector<klee::ConstraintManager>
  rename(const std::vector<klee::ConstraintManager> &constraints_list) {
    std::vector<klee::ConstraintManager> renamed_constraints_list;

    for (auto constraints : constraints_list) {
      klee::ConstraintManager renamed_constraints;

      for (auto constraint : constraints) {
        clear_replacements();

        auto renamed_constraint = rename(constraint);
        renamed_constraints.addConstraint(renamed_constraint);
      }

      renamed_constraints_list.push_back(renamed_constraints);
    }

    return renamed_constraints_list;
  }

  klee::ExprVisitor::Action visitExprPost(const klee::Expr &e) {
    auto eref = klee::ref<klee::Expr>(const_cast<klee::Expr *>(&e));
    auto it =
        replacements.find(klee::ref<klee::Expr>(const_cast<klee::Expr *>(&e)));

    if (it != replacements.end()) {
      return Action::changeTo(it->second);
    } else {
      return Action::doChildren();
    }
  }

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    auto ul = e.updates;
    auto root = ul.root;
    auto symbol = root->getName();

    auto found_it = translations.find(symbol);

    if (found_it != translations.end()) {
      auto replaced = klee::expr::ExprHandle(const_cast<klee::ReadExpr *>(&e));
      auto it = replacements.find(replaced);

      if (it == replacements.end()) {
        auto new_root = solver_toolbox.arr_cache.CreateArray(
            found_it->second, root->getSize(),
            root->constantValues.begin().base(),
            root->constantValues.end().base(), root->getDomain(),
            root->getRange());

        auto new_ul = klee::UpdateList(new_root, ul.head);
        auto replacement = solver_toolbox.exprBuilder->Read(new_ul, e.index);

        replacements.insert({replaced, replacement});

        return Action::changeTo(replacement);
      }
    }

    return Action::doChildren();
  }
};

} // namespace kutil