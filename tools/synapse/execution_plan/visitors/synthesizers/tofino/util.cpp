#include "util.h"
#include "klee-util.h"
#include "tofino_generator.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string p4_type_from_expr(klee::ref<klee::Expr> expr) {
  auto size_bits = expr->getWidth();
  std::stringstream label;
  label << "bit<" << size_bits << ">";
  return label.str();
}

std::vector<key_var_t> get_key_vars(Ingress &ingress, klee::ref<klee::Expr> key,
                                    const std::vector<meta_t> &meta) {
  std::vector<key_var_t> key_vars;
  auto free_byte_counter = 0u;

  auto size_bits = key->getWidth();
  auto offset_bits = 0u;

  while (offset_bits < size_bits) {
    auto found = false;

    for (const auto &meta : meta) {
      if (meta.offset != offset_bits) {
        continue;
      }

      auto meta_varq = ingress.search_variable(meta.symbol);

      if (!meta_varq.valid) {
        continue;
      }

      // Careful here, don't go find the size of this by
      // meta_varq.var->get_size_bits(). That method gives the P4 size. That may
      // differ from the expression size, for example in the case of ports (16
      // bits vs 9 bits).
      auto meta_size = meta.size;

      if (size_bits - offset_bits < meta_size) {
        continue;
      }

      auto key_var = key_var_t{*meta_varq.var, offset_bits, false};
      key_vars.push_back(key_var);
      offset_bits += meta_size;
      found = true;
      break;
    }

    if (found) {
      continue;
    }

    auto key_byte =
        kutil::solver_toolbox.exprBuilder->Extract(key, offset_bits, 8);

    auto hdr_varq = ingress.headers.query_hdr_field_from_chunk(key_byte);

    if (hdr_varq.valid && hdr_varq.offset_bits == 0) {
      auto hdr_size = hdr_varq.var->get_size_bits();

      if (size_bits - offset_bits >= hdr_size) {
        auto hdr_expr = hdr_varq.var->get_expr();
        auto hdr_chunk = kutil::solver_toolbox.exprBuilder->Extract(
            key, offset_bits, hdr_size);
        auto eq =
            kutil::solver_toolbox.are_exprs_always_equal(hdr_expr, hdr_chunk);

        if (eq) {
          auto key_var = key_var_t{*hdr_varq.var, offset_bits, false};
          key_vars.push_back(key_var);
          offset_bits += hdr_size;
          continue;
        }
      }
    }

    auto key_byte_var = ingress.allocate_key_byte(free_byte_counter);
    auto key_var = key_var_t{key_byte_var, offset_bits, true};
    key_vars.push_back(key_var);
    offset_bits += 8;
    free_byte_counter++;
  }

  return key_vars;
}

int get_num_packet_bytes(klee::ref<klee::Expr> expr) {
  kutil::RetrieveSymbols retriever;
  retriever.visit(expr);

  auto chunks = retriever.get_retrieved_packet_chunks();
  return chunks.size();
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

klee::ref<klee::Expr> simplify(klee::ref<klee::Expr> expr) {
  klee::ref<klee::Expr> out;

  if (simplify_cmp_eq_0(expr, out)) {
    return out;
  }

  return expr;
}

std::vector<klee::ref<klee::Expr>> split_condition(klee::ref<klee::Expr> expr) {
  std::vector<klee::ref<klee::Expr>> conditions{expr};

  auto and_expr_finder = [](klee::ref<klee::Expr> e) {
    return e->getKind() == klee::Expr::And;
  };

  while (1) {
    auto found_it =
        std::find_if(conditions.begin(), conditions.end(), and_expr_finder);

    if (found_it == conditions.end()) {
      break;
    }

    auto and_expr = *found_it;
    conditions.erase(found_it);

    auto lhs = and_expr->getKid(0);
    auto rhs = and_expr->getKid(1);

    conditions.push_back(lhs);
    conditions.push_back(rhs);
  }

  // minor simplification step

  for (auto &condition : conditions) {
    if (condition->getKind() == klee::Expr::ZExt ||
        condition->getKind() == klee::Expr::SExt) {
      condition = condition->getKid(0);
    }
  }

  return conditions;
}

} // namespace tofino
} // namespace synthesizer
} // namespace synapse