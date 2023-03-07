#pragma once

#include "../module.h"
#include "ignore.h"

namespace synapse {
namespace targets {
namespace tofino {

class TableLookup : public Module {
public:
  struct key_t {
    klee::ref<klee::Expr> expr;
    klee::ref<klee::Expr> condition;

    key_t(klee::ref<klee::Expr> _expr, klee::ref<klee::Expr> _condition)
        : expr(_expr), condition(_condition) {}

    key_t(klee::ref<klee::Expr> _expr) : expr(_expr) {}
  };

  typedef klee::ref<klee::Expr> param_t;

private:
  uint64_t table_id;
  klee::ref<klee::Expr> obj;
  std::vector<key_t> keys;
  std::vector<param_t> params;
  std::string contains_symbol;
  std::string bdd_function;

public:
  TableLookup()
      : Module(ModuleType::Tofino_TableLookup, Target::Tofino, "TableLookup") {}

  TableLookup(BDD::BDDNode_ptr node, uint64_t _table_id,
              klee::ref<klee::Expr> _obj, const std::vector<key_t> &_keys,
              const std::vector<param_t> &_params,
              const std::string &_contains_symbol,
              const std::string &_bdd_function)
      : Module(ModuleType::Tofino_TableLookup, Target::Tofino, "TableLookup",
               node),
        table_id(_table_id), obj(_obj), keys(_keys), params(_params),
        contains_symbol(_contains_symbol), bdd_function(_bdd_function) {}

private:
  bool multiple_queries_to_this_table(BDD::BDDNode_ptr current_node,
                                      uint64_t _table_id) const {
    assert(current_node);
    auto node = current_node->get_prev();

    unsigned int counter = 0;

    while (node) {
      if (node->get_type() != BDD::Node::NodeType::CALL) {
        node = node->get_prev();
        continue;
      }

      auto call_node = static_cast<const BDD::Call *>(node.get());
      auto call = call_node->get_call();

      if (call.function_name != symbex::FN_MAP_GET &&
          call.function_name != symbex::FN_MAP_PUT &&
          call.function_name != symbex::FN_VECTOR_BORROW) {
        node = node->get_prev();
        continue;
      }

      uint64_t this_table_id = 0;
      if (call.function_name == symbex::FN_MAP_GET ||
          call.function_name == symbex::FN_MAP_PUT) {
        assert(!call.args[symbex::FN_MAP_ARG_MAP].expr.isNull());
        this_table_id = kutil::solver_toolbox.value_from_expr(
            call.args[symbex::FN_MAP_ARG_MAP].expr);
      } else if (call.function_name == symbex::FN_VECTOR_BORROW) {
        assert(!call.args[symbex::FN_VECTOR_ARG_VECTOR].expr.isNull());
        this_table_id = kutil::solver_toolbox.value_from_expr(
            call.args[symbex::FN_VECTOR_ARG_VECTOR].expr);
      }

      if (this_table_id == _table_id) {
        counter++;
      }

      if (counter > 1) {
        return true;
      }

      node = node->get_prev();
    }

    return false;
  }

  struct extracted_data_t {
    bool valid;
    std::string fname;
    klee::ref<klee::Expr> obj;
    klee::ref<klee::Expr> key;
    klee::ref<klee::Expr> value;
    std::string contains_symbol;

    extracted_data_t(const std::string &_fname) : valid(false), fname(_fname) {}
  };

  extracted_data_t extract_from_map(const ExecutionPlan &ep,
                                    BDD::BDDNode_ptr node,
                                    const BDD::Call *casted) {
    auto call = casted->get_call();
    extracted_data_t data(call.function_name);

    if (call.function_name != symbex::FN_MAP_GET) {
      return data;
    }

    assert(call.function_name == symbex::FN_MAP_GET);
    assert(!call.args[symbex::FN_MAP_ARG_MAP].expr.isNull());
    assert(!call.args[symbex::FN_MAP_ARG_KEY].in.isNull());
    assert(!call.args[symbex::FN_MAP_ARG_OUT].out.isNull());

    auto _map = call.args[symbex::FN_MAP_ARG_MAP].expr;
    auto _key = call.args[symbex::FN_MAP_ARG_KEY].in;
    auto _value = call.args[symbex::FN_MAP_ARG_OUT].out;

    auto symbols = casted->get_generated_symbols();
    assert(symbols.size() == 2);

    auto symbols_it = symbols.begin();
    assert(symbols_it->label_base == symbex::MAP_HAS_THIS_KEY);
    auto _map_has_this_key_label = symbols_it->label;

    data.valid = true;
    data.obj = _map;
    data.key = _key;
    data.value = _value;
    data.contains_symbol = _map_has_this_key_label;

    return data;
  }

  extracted_data_t extract_from_vector(const ExecutionPlan &ep,
                                       BDD::BDDNode_ptr node,
                                       const BDD::Call *casted) {
    auto call = casted->get_call();
    extracted_data_t data(call.function_name);

    if (call.function_name != symbex::FN_VECTOR_BORROW) {
      return data;
    }

    assert(call.function_name == symbex::FN_VECTOR_BORROW);
    assert(!call.args[symbex::FN_VECTOR_ARG_VECTOR].expr.isNull());
    assert(!call.args[symbex::FN_VECTOR_ARG_INDEX].expr.isNull());
    assert(!call.extra_vars[symbex::FN_VECTOR_EXTRA].second.isNull());

    auto _vector = call.args[symbex::FN_VECTOR_ARG_VECTOR].expr;
    auto _index = call.args[symbex::FN_VECTOR_ARG_INDEX].expr;
    auto _borrowed_cell = call.extra_vars[symbex::FN_VECTOR_EXTRA].second;

    data.valid = true;
    data.obj = _vector;
    data.key = _index;
    data.value = _borrowed_cell;

    return data;
  }

  bool process(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
               const BDD::Call *casted, processing_result_t &result) {
    auto data = extract_from_map(ep, node, casted);

    if (!data.valid) {
      data = extract_from_vector(ep, node, casted);

      if (!data.valid) {
        return false;
      }
    }

    assert(data.obj->getKind() == klee::Expr::Kind::Constant);
    auto _obj_value = kutil::solver_toolbox.value_from_expr(data.obj);

    if (multiple_queries_to_this_table(node, _obj_value)) {
      return false;
    }

    auto _table_id = node->get_id();

    std::vector<key_t> _keys;

    if (ep.can_recall<klee::ref<klee::Expr>>(node->get_id())) {
      auto _key_condition = ep.recall<klee::ref<klee::Expr>>(node->get_id());
      _keys.emplace_back(data.key, _key_condition);
    } else {
      _keys.emplace_back(data.key);
    }

    std::vector<param_t> _params{data.value};
    std::string _contains_symbol = data.contains_symbol;

    auto new_module =
        std::make_shared<TableLookup>(node, _table_id, data.obj, _keys, _params,
                                      _contains_symbol, data.fname);

    auto new_ep = ep.add_leaves(new_module, node->get_next());

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return true;
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    process(ep, node, casted, result);
    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new TableLookup(node, table_id, obj, keys, params,
                                  contains_symbol, bdd_function);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const TableLookup *>(other);

    if (table_id != other_cast->get_table_id()) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(obj, other_cast->obj)) {
      return false;
    }

    auto other_keys = other_cast->get_keys();

    if (keys.size() != other_keys.size()) {
      return false;
    }

    for (auto i = 0u; i < keys.size(); i++) {
      if (!kutil::solver_toolbox.are_exprs_always_equal(keys[i].expr,
                                                        other_keys[i].expr)) {
        return false;
      }
    }

    auto other_params = other_cast->get_params();

    if (params.size() != other_params.size()) {
      return false;
    }

    for (auto i = 0u; i < params.size(); i++) {
      if (!kutil::solver_toolbox.are_exprs_always_equal(params[i],
                                                        other_params[i])) {
        return false;
      }
    }

    auto other_contains_symbol = other_cast->get_contains_symbol();

    if (contains_symbol != other_contains_symbol) {
      return false;
    }

    if (bdd_function != other_cast->get_bdd_function()) {
      return false;
    }

    return true;
  }

  uint64_t get_table_id() const { return table_id; }
  const klee::ref<klee::Expr> &get_obj() const { return obj; }
  const std::vector<key_t> &get_keys() const { return keys; }
  const std::vector<param_t> &get_params() const { return params; }
  const std::string &get_contains_symbol() const { return contains_symbol; }
  const std::string &get_bdd_function() const { return bdd_function; }
};
} // namespace tofino
} // namespace targets
} // namespace synapse
