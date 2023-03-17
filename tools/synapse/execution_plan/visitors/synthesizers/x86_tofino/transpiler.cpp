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
try_transpile_constant(x86TofinoGenerator &tg,
                       const klee::ref<klee::Expr> &expr) {
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
try_transpile_variable(x86TofinoGenerator &tg,
                       const klee::ref<klee::Expr> &expr) {
  auto result = std::pair<bool, std::string>();
  auto variable = tg.search_variable(expr);

  if (!variable.valid) {
    return result;
  }

  std::stringstream transpilation_builder;
  auto label = variable.var->get_label();

  auto size_bits = expr->getWidth();

  if (variable.offset_bits > 0 || size_bits < variable.var->get_size_bits()) {
    auto mask = 0llu;
    for (auto b = 0u; b < expr->getWidth(); b++) {
      mask <<= 1;
      mask |= 1;
    }

    assert(mask > 0);

    transpilation_builder << "(";
    transpilation_builder << "(";
    transpilation_builder << label;
    transpilation_builder << " >> ";
    transpilation_builder << variable.offset_bits;
    transpilation_builder << ")";
    transpilation_builder << " & 0x";
    transpilation_builder << std::hex;
    transpilation_builder << mask;
    transpilation_builder << std::dec;
    transpilation_builder << ")";
  } else {
    transpilation_builder << label;
  }

  result.first = true;
  result.second = transpilation_builder.str();

  return result;
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
  klee::ref<klee::Expr> eref = const_cast<klee::ConcatExpr *>(&e);

  if (kutil::is_readLSB(eref)) {
    auto symbol = kutil::get_symbol(eref);
    auto variable = tg.search_variable(symbol.second);

    if (!variable.valid) {
      Log::err() << "Unknown variable with symbol " << symbol.second << "\n";
      exit(1);
    }

    assert(variable.offset_bits == 0);

    auto expr_width = eref->getWidth();
    auto var_width = variable.var->get_size_bits();

    if (expr_width != var_width) {
      auto max_width = std::max(expr_width, var_width);
      code << "(";
      code << "(bit<";
      code << max_width;
      code << ">)";
      code << variable.var->get_label();
      code << ")";
    } else {
      code << variable.var->get_label();
    }

    return klee::ExprVisitor::Action::skipChildren();
  }

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
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " == ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
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