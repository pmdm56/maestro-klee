#include "transpiler.h"
#include "../../../../log.h"
#include "../util.h"
#include "klee-util.h"
#include "x86_tofino_generator.h"

#include <assert.h>

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

class InternalTranspiler : public klee::ExprVisitor::ExprVisitor {
private:
  x86TofinoGenerator &tg;
  Transpiler &t;
  std::stringstream code;

public:
  InternalTranspiler(x86TofinoGenerator &_tg, Transpiler &_t)
      : tg(_tg), t(_t) {}

  std::string get() const { return code.str(); }

private:
  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e);
  klee::ExprVisitor::Action visitSelect(const klee::SelectExpr &e);
  klee::ExprVisitor::Action visitConcat(const klee::ConcatExpr &e);
  klee::ExprVisitor::Action visitExtract(const klee::ExtractExpr &e);
  klee::ExprVisitor::Action visitZExt(const klee::ZExtExpr &e);
  klee::ExprVisitor::Action visitSExt(const klee::SExtExpr &e);
  klee::ExprVisitor::Action visitAdd(const klee::AddExpr &e);
  klee::ExprVisitor::Action visitSub(const klee::SubExpr &e);
  klee::ExprVisitor::Action visitMul(const klee::MulExpr &e);
  klee::ExprVisitor::Action visitUDiv(const klee::UDivExpr &e);
  klee::ExprVisitor::Action visitSDiv(const klee::SDivExpr &e);
  klee::ExprVisitor::Action visitURem(const klee::URemExpr &e);
  klee::ExprVisitor::Action visitSRem(const klee::SRemExpr &e);
  klee::ExprVisitor::Action visitNot(const klee::NotExpr &e);
  klee::ExprVisitor::Action visitAnd(const klee::AndExpr &e);
  klee::ExprVisitor::Action visitOr(const klee::OrExpr &e);
  klee::ExprVisitor::Action visitXor(const klee::XorExpr &e);
  klee::ExprVisitor::Action visitShl(const klee::ShlExpr &e);
  klee::ExprVisitor::Action visitLShr(const klee::LShrExpr &e);
  klee::ExprVisitor::Action visitAShr(const klee::AShrExpr &e);
  klee::ExprVisitor::Action visitEq(const klee::EqExpr &e);
  klee::ExprVisitor::Action visitNe(const klee::NeExpr &e);
  klee::ExprVisitor::Action visitUlt(const klee::UltExpr &e);
  klee::ExprVisitor::Action visitUle(const klee::UleExpr &e);
  klee::ExprVisitor::Action visitUgt(const klee::UgtExpr &e);
  klee::ExprVisitor::Action visitUge(const klee::UgeExpr &e);
  klee::ExprVisitor::Action visitSlt(const klee::SltExpr &e);
  klee::ExprVisitor::Action visitSle(const klee::SleExpr &e);
  klee::ExprVisitor::Action visitSgt(const klee::SgtExpr &e);
  klee::ExprVisitor::Action visitSge(const klee::SgeExpr &e);
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

  auto transpiler = InternalTranspiler(tg, *this);
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

std::string Transpiler::size_to_type(bits_t size) const {
  assert(size > 0);
  assert(size % 8 == 0);

  if (is_primitive_type(size)) {
    std::stringstream type_builder;
    type_builder << "uint";
    type_builder << size;
    type_builder << "_t";
    return type_builder.str();
  }

  return "bytes_t";
}

klee::ExprVisitor::Action
InternalTranspiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::ReadExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitSelect(const klee::SelectExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SelectExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
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
      code << "((";
      code << t.size_to_type(max_width);
      code << ")(";
      code << variable.var->get_label();
      code << "))";
    } else {
      code << variable.var->get_label();
    }

    return klee::ExprVisitor::Action::skipChildren();
  }

  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitExtract(const klee::ExtractExpr &e) {
  auto expr = e.expr;
  auto offset = e.offset;
  // auto sz = e.width;

  // capture (Extract 0 (ZExt w8 X))
  if (offset == 0 && expr->getKind() == klee::Expr::Kind::ZExt &&
      expr->getWidth() == klee::Expr::Int8) {
    assert(expr->getNumKids() == 1);
    auto zextended_expr = expr->getKid(0);
    code << tg.transpile(zextended_expr);
    return klee::ExprVisitor::Action::skipChildren();
  }

  klee::ref<klee::Expr> eref = const_cast<klee::ExtractExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitZExt(const klee::ZExtExpr &e) {
  auto sz = e.getWidth();
  auto expr = e.getKid(0);
  assert(sz % 8 == 0);

  code << "((";
  code << t.size_to_type(sz);
  code << ")(";
  code << tg.transpile(expr);
  code << "))";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action
InternalTranspiler::visitSExt(const klee::SExtExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SExtExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitAdd(const klee::AddExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " + ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitSub(const klee::SubExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " - ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitMul(const klee::MulExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " * ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action
InternalTranspiler::visitUDiv(const klee::UDivExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " / ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action
InternalTranspiler::visitSDiv(const klee::SDivExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SDivExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitURem(const klee::URemExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " % ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action
InternalTranspiler::visitSRem(const klee::SRemExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SRemExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitNot(const klee::NotExpr &e) {
  assert(e.getNumKids() == 1);

  auto arg = e.getKid(0);
  auto arg_parsed = tg.transpile(arg);

  code << "!(" << arg_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitAnd(const klee::AndExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " & ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitOr(const klee::OrExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " | ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitXor(const klee::XorExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " ^ ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitShl(const klee::ShlExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::ShlExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitLShr(const klee::LShrExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::LShrExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action
InternalTranspiler::visitAShr(const klee::AShrExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::AShrExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
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

klee::ExprVisitor::Action InternalTranspiler::visitNe(const klee::NeExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " != ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitUlt(const klee::UltExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " < ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitUle(const klee::UleExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " <= ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitUgt(const klee::UgtExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " > ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitUge(const klee::UgeExpr &e) {
  assert(e.getNumKids() == 2);

  auto lhs = e.getKid(0);
  auto rhs = e.getKid(1);

  auto lhs_parsed = tg.transpile(lhs);
  auto rhs_parsed = tg.transpile(rhs);

  code << "(" << lhs_parsed << ")";
  code << " >= ";
  code << "(" << rhs_parsed << ")";

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action InternalTranspiler::visitSlt(const klee::SltExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SltExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSle(const klee::SleExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SleExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSgt(const klee::SgtExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SgtExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

klee::ExprVisitor::Action InternalTranspiler::visitSge(const klee::SgeExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::SgeExpr *>(&e);
  std::cerr << kutil::expr_to_string(eref) << "\n";
  assert(false && "TODO");
}

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse