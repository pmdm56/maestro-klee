#include "klee_transpiler.h"

Type_ptr klee_width_to_type(klee::Expr::Width width) {
  assert(width != klee::Expr::InvalidWidth);
  return type_from_size(width);
}

Constant_ptr const_to_ast_expr(const klee::ref<klee::Expr> &e) {
  assert(!e.isNull());

  if (e->getKind() != klee::Expr::Kind::Constant) {
    return nullptr;
  }

  klee::ConstantExpr* constant = static_cast<klee::ConstantExpr *>(e.get());
  Type_ptr type = klee_width_to_type(constant->getWidth());

  Constant_ptr constant_node = Constant::build(type);

  if (type->get_type_kind() == Type::TypeKind::ARRAY) {
    Array* array = static_cast<Array*>(type.get());

    for (unsigned int offset = 0; offset < array->get_n_elems(); offset++) {
      auto byte = ast_builder_assistant_t::exprBuilder->Extract(constant, offset * 8, 8);
      auto value = ast_builder_assistant_t::value_from_expr(byte);
      constant_node->set_value(value, offset);
    }
  }

  else {
    assert(type->get_size() <= 64);
    uint64_t value = constant->getZExtValue();
    constant_node->set_value(value);
  }

  return constant_node;
}

Expr_ptr transpile(AST* ast, const klee::ref<klee::Expr> &e) {
  Expr_ptr result = const_to_ast_expr(e);

  if (result) {
    return result;
  }

  result = ast->get_from_local(e);

  if (result) {
    return result;
  }

  KleeExprToASTNodeConverter converter(ast);
  converter.visit(e);

  result = converter.get_result();
  assert(result);

  return result->simplify(ast);
}

uint64_t get_first_concat_idx(const klee::ref<klee::Expr> &e) {
  assert(e->getKind() == klee::Expr::Kind::Concat);

  klee::ref<klee::Expr> curr_node = e;
  while (curr_node->getKind() == klee::Expr::Kind::Concat) {
    curr_node = curr_node->getKid(1);
  }

  assert(curr_node->getKind() == klee::Expr::Kind::Read);
  klee::ReadExpr* read = static_cast<klee::ReadExpr*>(curr_node.get());

  Expr_ptr idx = const_to_ast_expr(read->index);
  assert(idx->get_kind() == Node::NodeKind::CONSTANT);

  Constant* idx_const = static_cast<Constant*>(idx.get());
  return idx_const->get_value();
}

uint64_t get_last_concat_idx(const klee::ref<klee::Expr> &e) {
  assert(e->getKind() == klee::Expr::Kind::Concat);

  klee::ref<klee::Expr> left = e->getKid(0);

  assert(left->getKind() == klee::Expr::Kind::Read);
  klee::ReadExpr* read = static_cast<klee::ReadExpr*>(left.get());

  Expr_ptr idx = const_to_ast_expr(read->index);
  assert(idx->get_kind() == Node::NodeKind::CONSTANT);

  Constant* idx_const = static_cast<Constant*>(idx.get());
  return idx_const->get_value();
}

std::vector<Expr_ptr> apply_changes_to_match(AST *ast,
                                             const klee::ref<klee::Expr> &before,
                                             const klee::ref<klee::Expr> &after) {
  assert(before->getWidth() == after->getWidth());

  std::vector<Expr_ptr> changes;

  Expr_ptr before_expr = transpile(ast, before);
  Expr_ptr after_expr = transpile(ast, after);

  Type_ptr type = before_expr->get_type();

  while (type->get_type_kind() == Type::TypeKind::POINTER) {
    Pointer* ptr = static_cast<Pointer*>(before_expr->get_type().get());
    type = ptr->get_type();
  }

  switch (type->get_type_kind()) {
  case Type::TypeKind::STRUCT: {
    Struct* s = static_cast<Struct*>(type.get());
    std::vector<Variable_ptr> fields = s->get_fields();

    unsigned int offset = 0;
    for (auto field : fields) {
      auto field_size = field->get_type()->get_size();

      auto e1_chunk = ast_builder_assistant_t::exprBuilder->Extract(before, offset, field_size);
      auto e2_chunk = ast_builder_assistant_t::exprBuilder->Extract(after, offset, field_size);

      bool eq = ast_builder_assistant_t::are_exprs_always_equal(e1_chunk, e2_chunk);

      if (!eq) {
        auto field_changes = apply_changes_to_match(ast, e1_chunk, e2_chunk);
        changes.insert(changes.end(), field_changes.begin(), field_changes.end());
      }

      offset += field_size;
    }

    break;
  }
  case Type::TypeKind::PRIMITIVE: {
    assert(false && "Not implemented");
    break;
  }
  case Type::TypeKind::ARRAY: {
    Array* a = static_cast<Array*>(type.get());
    auto elem_sz = a->get_elem_type()->get_size();

    for (unsigned int offset = 0; offset < a->get_size(); offset += elem_sz) {
      auto e1_chunk = ast_builder_assistant_t::exprBuilder->Extract(before, offset, elem_sz);
      auto e2_chunk = ast_builder_assistant_t::exprBuilder->Extract(after, offset, elem_sz);

      bool eq = ast_builder_assistant_t::are_exprs_always_equal(e1_chunk, e2_chunk);

      if (!eq) {
        Expr_ptr e1_chunk_ast = transpile(ast, e1_chunk);
        Expr_ptr e2_chunk_ast = transpile(ast, e2_chunk);

        Expr_ptr change = Assignment::build(e1_chunk_ast, e2_chunk_ast);
        changes.push_back(change->simplify(ast));
      }
    }

    break;
  }
  case Type::TypeKind::POINTER:
    assert(false && "Should not be here");
    break;
  }

  return changes;
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> eref = const_cast<klee::ReadExpr *>(&e);

  Type_ptr type = klee_width_to_type(e.getWidth());
  Expr_ptr idx = transpile(ast, e.index);

  klee::UpdateList ul = e.updates;
  const klee::Array *root = ul.root;
  std::string symbol = root->name;

  if (symbol == "VIGOR_DEVICE") {
    symbol = "src_devices";
  }

  else if (symbol == "next_time") {
    symbol = "now";
  }

  else if (symbol == "data_len") {
    symbol = "pkt_len";
  }

  else if (symbol == "packet_chunks") {
    assert(idx->get_kind() == Node::NodeKind::CONSTANT);
    Constant* idx_const = static_cast<Constant*>(idx.get());

    AST::chunk_t chunk_info = ast->get_chunk_from_local(idx_const->get_value());
    Variable_ptr var = chunk_info.var;
    assert(var != nullptr);

    unsigned new_idx_value = idx_const->get_value() - chunk_info.start_index;

    PrimitiveType* p = static_cast<PrimitiveType*>(idx_const->get_type().get());
    Constant_ptr new_idx = Constant::build(p->get_primitive_kind(), new_idx_value);

    Read_ptr read = Read::build(var, type, new_idx);
    assert(read);

    save_result(read);

    return klee::ExprVisitor::Action::skipChildren();
  }

  symbol_width = std::make_pair(true, root->getSize() * 8);

  Expr_ptr var = ast->get_from_local(symbol);

  if (var == nullptr) {
    var = ast->get_from_local(eref);

    if (var == nullptr) {
      ast->dump_stack();

      std::cerr << "\n";
      std::cerr << "Variable with symbol '" << symbol << "' not found:" << "\n";
      std::cerr << expr_to_string(eref) << "\n";
      std::cerr << "\n";

      assert(var != nullptr);
    }
  }

  Read_ptr read = Read::build(var, type, idx);
  assert(read);

  save_result(read);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSelect(const klee::SelectExpr& e) {
  assert(e.getNumKids() == 3);

  Expr_ptr cond = transpile(ast, e.getKid(0));
  assert(cond);

  Expr_ptr first = transpile(ast, e.getKid(1));
  assert(first);

  Expr_ptr second = transpile(ast, e.getKid(2));
  assert(second);

  Select_ptr select = Select::build(cond, first, second);

  save_result(select);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitConcat(const klee::ConcatExpr& e) {
  Expr_ptr left = transpile(ast, e.getKid(0));
  Expr_ptr right = transpile(ast, e.getKid(1));
  Type_ptr type = klee_width_to_type(e.getWidth());

  if (left->get_kind() == Node::NodeKind::READ) {
    Read* lread = static_cast<Read*>(left.get());

    if (lread->get_idx()->get_kind() == Node::NodeKind::CONSTANT) {
      Constant* lread_idx = static_cast<Constant*>(lread->get_idx().get());

      auto left_byte_offset = lread->get_type()->get_size() * lread_idx->get_value();
      auto left_size = lread->get_expr()->get_type()->get_size();

      if (left_byte_offset >= left_size) {
        save_result(right->simplify(ast));
        return klee::ExprVisitor::Action::skipChildren();
      }
    }
  }

  Concat_ptr concat = Concat::build(left, right, type);

  RetrieveSymbols retriever;
  retriever.visit(klee::ref<klee::Expr>(const_cast<klee::ConcatExpr *>(&e)));
  auto symbols = retriever.get_retrieved_strings();

  if (symbols.size() != 1) {
    save_result(concat);
    return klee::ExprVisitor::Action::skipChildren();
  }

  Expr_ptr simplified = concat->simplify(ast);

  save_result(simplified);
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitExtract(const klee::ExtractExpr& e) {
  auto expr = e.getKid(0);
  auto offset_value = e.offset;
  auto size = e.width;

  assert(offset_value % 8 == 0);

  Type_ptr type = klee_width_to_type(e.getWidth());

  Expr_ptr ast_expr = transpile(ast, expr);
  assert(ast_expr);

  while (ast_expr->get_kind() == Node::NodeKind::CONCAT) {
    Concat* concat = static_cast<Concat*>(ast_expr.get());

    Expr_ptr left = concat->get_left();
    Expr_ptr right = concat->get_right();

    auto right_size = right->get_type()->get_size();

    if (offset_value < right_size && offset_value < right_size + size) {
      ast_expr = right;
    }

    else if (offset_value >= right_size && offset_value >= right_size + size) {
      ast_expr = left;
      offset_value -= right_size;
    }

    else {
      break;
    }
  }

  if (ast_expr->get_kind() == Node::NodeKind::VARIABLE) {
    Expr_ptr offset = Constant::build(PrimitiveType::PrimitiveKind::UINT64_T, offset_value / 8);
    Read_ptr read = Read::build(ast_expr, type, offset);

    save_result(read);
    return klee::ExprVisitor::Action::skipChildren();
  }

  if (ast_expr->get_kind() == Node::NodeKind::CONSTANT) {
    Constant* constant = static_cast<Constant*>(ast_expr.get());
    switch (constant->get_type()->get_type_kind()) {
    case Type::TypeKind::PRIMITIVE: {
      Constant_ptr new_constant = Constant::build(type);
      new_constant->set_value((constant->get_value() >> (offset_value)) & ((1 << size) - 1));

      save_result(new_constant);
      return klee::ExprVisitor::Action::skipChildren();
    }
    case Type::TypeKind::POINTER:
    case Type::TypeKind::STRUCT:
      assert(false && "Not implemented");
      break;
    case Type::TypeKind::ARRAY: {
      Array* array = static_cast<Array*>(constant->get_type().get());

      assert(offset_value % array->get_elem_type()->get_size() == 0);
      assert(size % array->get_elem_type()->get_size() == 0);

      unsigned int new_size = size / array->get_elem_type()->get_size();
      unsigned int old_idx = offset_value / array->get_elem_type()->get_size();
      unsigned int new_idx = 0;

      if (new_size == 1) {
        Constant_ptr new_constant = Constant::build(array->get_elem_type());
        new_constant->set_value(constant->get_value(old_idx));

        save_result(new_constant);
        return klee::ExprVisitor::Action::skipChildren();
      }

      Array_ptr new_array = Array::build(array->get_elem_type(), new_size);
      Constant_ptr new_constant = Constant::build(new_array);

      while (size > 0) {
        new_constant->set_value(constant->get_value(old_idx), new_idx);

        size -= array->get_elem_type()->get_size();
        new_idx++;
        old_idx++;
      }

      save_result(new_constant);
      return klee::ExprVisitor::Action::skipChildren();
    }
    }
  }

  Expr_ptr mask = Constant::build(PrimitiveType::PrimitiveKind::UINT64_T, (1 << size) - 1, true);
  Expr_ptr extract;

  if (offset_value > 0) {
    Expr_ptr offset = Constant::build(PrimitiveType::PrimitiveKind::UINT64_T, offset_value);
    ShiftRight_ptr shift = ShiftRight::build(ast_expr, offset);
    extract = And::build(shift, mask);
  }

  else {
    extract = ast_expr;
  }

  Cast_ptr cast = Cast::build(extract, type);

  save_result(cast);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitZExt(const klee::ZExtExpr& e) {
  assert(e.getNumKids() == 1);

  Type_ptr type = klee_width_to_type(e.getWidth());
  auto expr = e.getKid(0);

  Expr_ptr ast_expr = transpile(ast, expr);
  assert(ast_expr);

  Cast_ptr cast = Cast::build(ast_expr, type);

  save_result(cast);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSExt(const klee::SExtExpr& e) {
  assert(e.getNumKids() == 1);

  auto size = e.getWidth();
  auto expr_size = e.getKid(0)->getWidth();

  Type_ptr type = klee_width_to_type(e.getWidth());
  Expr_ptr expr = transpile(ast, e.getKid(0));
  assert(expr);

  unsigned int mask = 0;
  for (unsigned int i = 0; i < size; i++) {
    if (i < (size - expr_size)) {
      mask = (mask << 1) | 1;
    } else {
      mask = (mask << 1);
    }
  }

  Expr_ptr result;

  if (size > expr_size) {
    assert(type->get_type_kind() == Type::TypeKind::PRIMITIVE);

    PrimitiveType* primitive = static_cast<PrimitiveType*>(type.get());
    Expr_ptr mask_expr = Constant::build(primitive->get_primitive_kind(), mask, true);

    Expr_ptr shift_value = Constant::build(primitive->get_primitive_kind(), size - 1);
    ShiftRight_ptr msb = ShiftRight::build(expr, shift_value);

    Expr_ptr if_msb_one = Or::build(mask_expr, expr);
    Expr_ptr if_msb_not_one = Cast::build(expr, type);

    result = Select::build(msb, if_msb_one, if_msb_not_one);
  } else if (size == expr_size) {
    result = expr;
  } else {
    result = Cast::build(expr, type);

  }

  save_result(result);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitAdd(const klee::AddExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Add_ptr a = Add::build(left, right);
  save_result(a);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSub(const klee::SubExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Sub_ptr s = Sub::build(left, right);
  save_result(s);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitMul(const klee::MulExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Mul_ptr m = Mul::build(left, right);
  save_result(m);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitUDiv(const klee::UDivExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Div_ptr d = Div::build(left, right);
  save_result(d);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSDiv(const klee::SDivExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr cast = Cast::build(left, true);
  Div_ptr d = Div::build(cast, right);
  save_result(d);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitURem(const klee::URemExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Mod_ptr m = Mod::build(left, right);
  save_result(m);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSRem(const klee::SRemExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr cast = Cast::build(left, true);
  Mod_ptr m = Mod::build(cast, right);

  save_result(m);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitNot(const klee::NotExpr& e) {
  assert(e.getNumKids() == 1);

  Expr_ptr arg = transpile(ast, e.getKid(0));

  save_result(Not::build(arg));

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitAnd(const klee::AndExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  And_ptr a = And::build(left, right);
  save_result(a);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitOr(const klee::OrExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Or_ptr o = Or::build(left, right);
  save_result(o);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitXor(const klee::XorExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Xor_ptr x = Xor::build(left, right);
  save_result(x);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitShl(const klee::ShlExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  ShiftLeft_ptr sl = ShiftLeft::build(left, right);
  save_result(sl);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitLShr(const klee::LShrExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  ShiftRight_ptr sr = ShiftRight::build(left, right);
  save_result(sr);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitAShr(const klee::AShrExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr cast = Cast::build(left, true);
  ShiftRight_ptr sr = ShiftRight::build(cast, right);

  save_result(sr);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitEq(const klee::EqExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  if (right->get_kind() == Node::NodeKind::EQUALS &&
      left->get_kind() == Node::NodeKind::CONSTANT) {

    Constant* left_const = static_cast<Constant*>(left.get());
    Equals* right_eq = static_cast<Equals*>(right.get());
    Expr_ptr right_eq_left = right_eq->get_lhs();

    if (right_eq_left->get_kind() == Node::NodeKind::CONSTANT) {
      Constant* right_eq_left_const = static_cast<Constant*>(right_eq_left.get());

      if (right_eq_left_const->get_value() == 0 && left_const->get_value() == 0) {
        save_result(right_eq->get_rhs());
        return klee::ExprVisitor::Action::skipChildren();
      }
    }
  }

  Equals_ptr equals = Equals::build(left, right);
  save_result(equals);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitNe(const klee::NeExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  NotEquals_ptr ne = NotEquals::build(left, right);

  save_result(ne);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitUlt(const klee::UltExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Less_ptr lt = Less::build(left, right);

  save_result(lt);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitUle(const klee::UleExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  LessEq_ptr le = LessEq::build(left, right);

  save_result(le);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitUgt(const klee::UgtExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Greater_ptr gt = Greater::build(left, right);

  save_result(gt);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitUge(const klee::UgeExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  GreaterEq_ptr ge = GreaterEq::build(left, right);

  save_result(ge);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSlt(const klee::SltExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr lc = Cast::build(left, true);
  Cast_ptr rc = Cast::build(right, true);

  Less_ptr lt = Less::build(lc, rc);

  save_result(lt);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSle(const klee::SleExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr lc = Cast::build(left, true);
  Cast_ptr rc = Cast::build(right, true);

  LessEq_ptr le = LessEq::build(lc, rc);

  save_result(le);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSgt(const klee::SgtExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr lc = Cast::build(left, true);
  Cast_ptr rc = Cast::build(right, true);

  Greater_ptr gt = Greater::build(lc, rc);

  save_result(gt);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action KleeExprToASTNodeConverter::visitSge(const klee::SgeExpr& e) {
  assert(e.getNumKids() == 2);

  Expr_ptr left = transpile(ast, e.getKid(0));
  assert(left);

  Expr_ptr right = transpile(ast, e.getKid(1));
  assert(right);

  Cast_ptr lc = Cast::build(left, true);
  Cast_ptr rc = Cast::build(right, true);

  GreaterEq_ptr ge = GreaterEq::build(lc, rc);

  save_result(ge);

  return klee::ExprVisitor::Action::skipChildren();
}
