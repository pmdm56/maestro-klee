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
    std::vector<meta_t> meta;

    key_t(klee::ref<klee::Expr> _expr) : expr(_expr) {}
    key_t(klee::ref<klee::Expr> _expr, std::vector<meta_t> _meta)
        : expr(_expr), meta(_meta) {}
  };

  struct param_t {
    std::vector<klee::ref<klee::Expr>> exprs;

    param_t(klee::ref<klee::Expr> expr) { exprs.push_back(expr); }
  };

private:
  std::string table_name;
  std::vector<obj_addr_t> objs;
  std::vector<key_t> keys;
  std::vector<param_t> params;
  std::vector<BDD::symbol_t> contains_symbols;
  std::unordered_set<BDD::node_id_t> nodes;

public:
  TableLookup()
      : Module(ModuleType::Tofino_TableLookup, TargetType::Tofino,
               "TableLookup") {}

  TableLookup(BDD::BDDNode_ptr node, std::string _table_name,
              const std::vector<obj_addr_t> &_objs,
              const std::vector<key_t> &_keys,
              const std::vector<param_t> &_params,
              const std::vector<BDD::symbol_t> &_contains_symbols,
              const std::unordered_set<BDD::node_id_t> &_nodes)
      : Module(ModuleType::Tofino_TableLookup, TargetType::Tofino,
               "TableLookup", node),
        table_name(_table_name), objs(_objs), keys(_keys), params(_params),
        contains_symbols(_contains_symbols), nodes(_nodes) {}

private:
  struct extracted_data_t {
    bool valid;
    std::string fname;
    std::unordered_set<BDD::node_id_t> node_ids;
    std::vector<obj_addr_t> objs;
    std::vector<key_t> keys;
    std::vector<param_t> values;
    std::vector<BDD::symbol_t> contains_symbols;

    extracted_data_t() : valid(false) {}
    extracted_data_t(const std::string &_fname) : valid(false), fname(_fname) {}
  };

  extracted_data_t extract_from_dchain(const BDD::Call *casted) const {
    auto call = casted->get_call();
    extracted_data_t data(call.function_name);

    if (call.function_name != symbex::FN_DCHAIN_IS_ALLOCATED) {
      return data;
    }

    assert(!call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr.isNull());
    assert(!call.args[symbex::FN_DCHAIN_ARG_INDEX].expr.isNull());
    assert(!call.ret.isNull());

    auto _dchain = call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr;
    auto _index = call.args[symbex::FN_DCHAIN_ARG_INDEX].expr;
    auto _is_allocated = call.ret;

    auto symbols = casted->get_local_generated_symbols();
    assert(symbols.size() == 1);
    auto contains_symbol = *symbols.begin();

    auto _dchain_addr = kutil::expr_addr_to_obj_addr(_dchain);

    data.valid = true;
    data.node_ids.insert(casted->get_id());
    data.objs.push_back(_dchain_addr);
    data.keys.emplace_back(_index);
    data.contains_symbols.push_back(contains_symbol);

    return data;
  }

  extracted_data_t extract_from_map(const BDD::Call *casted) const {
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
    auto _key_meta = call.args[symbex::FN_MAP_ARG_KEY].meta;
    auto _value = call.args[symbex::FN_MAP_ARG_OUT].out;

    auto symbols = casted->get_local_generated_symbols();
    assert(symbols.size() == 2);

    auto symbols_it = symbols.begin();
    assert(symbols_it->label_base == symbex::MAP_HAS_THIS_KEY);
    auto _map_has_this_key = *symbols_it;

    auto _map_addr = kutil::expr_addr_to_obj_addr(_map);

    data.valid = true;
    data.node_ids.insert(casted->get_id());
    data.objs.push_back(_map_addr);
    data.keys.emplace_back(_key, _key_meta);
    data.values.emplace_back(_value);
    data.contains_symbols.push_back(_map_has_this_key);

    return data;
  }

  extracted_data_t extract_from_vector(const BDD::Call *casted) const {
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

    auto _vector_addr = kutil::expr_addr_to_obj_addr(_vector);

    data.valid = true;
    data.node_ids.insert(casted->get_id());
    data.objs.push_back(_vector_addr);
    data.keys.emplace_back(_index);
    data.values.emplace_back(_borrowed_cell);

    return data;
  }

  bool check_compatible_placements_decisions(
      const ExecutionPlan &ep, const std::vector<obj_addr_t> &objs) const {
    auto mb = ep.get_memory_bank();

    for (auto obj : objs) {
      if (!mb->has_placement_decision(obj)) {
        continue;
      }

      if (mb->check_placement_decision(obj, PlacementDecision::TofinoTable)) {
        continue;
      }

      return false;
    }

    return true;
  }

  void save_decision(const ExecutionPlan &ep,
                     const std::vector<obj_addr_t> &objs,
                     Module_ptr new_table) {
    auto mb = ep.get_memory_bank();

    for (const auto &obj : objs) {
      mb->save_placement_decision(obj, PlacementDecision::TofinoTable);

      auto tmb = ep.get_memory_bank<TofinoMemoryBank>(Tofino);
      tmb->save_table(obj, new_table);
    }
  }

  std::string build_table_name(const extracted_data_t &data) const {
    std::stringstream table_label_builder;

    table_label_builder << data.fname;

    for (auto node_id : data.node_ids) {
      table_label_builder << "_";
      table_label_builder << node_id;
    }

    return table_label_builder.str();
  }

  extracted_data_t extract_data(const BDD::Call *casted) const {
    auto extractors = {
        &TableLookup::extract_from_map,
        &TableLookup::extract_from_vector,
        &TableLookup::extract_from_dchain,
    };

    extracted_data_t data;

    for (auto &extractor : extractors) {
      data = (this->*extractor)(casted);

      if (data.valid) {
        break;
      }
    }

    return data;
  }

  void
  remember_to_ignore_coalesced(const ExecutionPlan &ep,
                               const coalesced_data_t &coalesced_data) const {
    auto mb = ep.get_memory_bank();

    for (auto incoming : coalesced_data.vector_borrows) {
      mb->can_be_ignored(incoming);
    }
  }

  void merge_coalesced_data(extracted_data_t &extracted_data,
                            const coalesced_data_t &coalesced_data) const {
    assert(coalesced_data.vector_borrows.size() > 0);

    for (auto vector : coalesced_data.vector_borrows) {
      assert(vector->get_type() == BDD::Node::CALL);
      auto call_node = static_cast<const BDD::Call *>(vector.get());
      auto vector_extracted_data = extract_data(call_node);

      extracted_data.node_ids.insert(vector_extracted_data.node_ids.begin(),
                                     vector_extracted_data.node_ids.end());

      extracted_data.objs.insert(extracted_data.objs.end(),
                                 vector_extracted_data.objs.begin(),
                                 vector_extracted_data.objs.end());

      extracted_data.values.insert(extracted_data.values.end(),
                                   vector_extracted_data.values.begin(),
                                   vector_extracted_data.values.end());
    }
  }

  bool try_merge_with_previous_table(const ExecutionPlan &ep,
                                     const BDD::Call *node,
                                     extracted_data_t &extracted_data) {
    assert(extracted_data.objs.size() == 1);
    auto obj = extracted_data.objs[0];

    auto tmb = ep.get_memory_bank<TofinoMemoryBank>(Tofino);
    auto have_tables = tmb->does_obj_have_tables(obj);

    if (!have_tables) {
      return false;
    }

    auto tables_modules = tmb->get_obj_tables(obj);

    // Find the right table to reuse
    for (auto table_module : tables_modules) {
      assert(table_module->get_type() == ModuleType::Tofino_TableLookup);
      auto table = static_cast<TableLookup *>(table_module.get());

      // The keys must have the same metadata
      auto other_keys = table->get_keys();

      if (extracted_data.keys.size() != other_keys.size()) {
        continue;
      }

      auto equal = true;

      for (auto i = 0u; i < extracted_data.keys.size(); i++) {
        auto this_meta = extracted_data.keys[i].meta;
        auto other_meta = other_keys[i].meta;

        if (this_meta.size() != other_meta.size()) {
          equal = false;
          break;
        }

        for (auto j = 0u; j < this_meta.size(); j++) {
          if (this_meta[i].symbol != other_meta[i].symbol ||
              this_meta[i].offset != other_meta[i].offset ||
              this_meta[i].size != other_meta[i].size) {
            equal = false;
            break;
          }
        }
      }

      if (!equal) {
        continue;
      }

      // Merge the contains symbols, so that the code generation can also find
      // information from this second table

      table->append_to_name(extracted_data.node_ids);
      table->add_contains_symbols(extracted_data.contains_symbols);
      table->add_nodes(extracted_data.node_ids);
      table->add_exprs_to_params(extracted_data.values);

      return true;
    }

    return false;
  }

  bool process(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
               const BDD::Call *casted, const extracted_data_t data,
               processing_result_t &result) {
    auto _table_name = build_table_name(data);

    auto new_module = std::make_shared<TableLookup>(
        node, _table_name, data.objs, data.keys, data.values,
        data.contains_symbols, data.node_ids);

    save_decision(ep, data.objs, new_module);

    auto new_ep = ep.add_leaves(new_module, node->get_next());

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return true;
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;

    auto extracted_data = extract_data(casted);

    if (!extracted_data.valid) {
      return result;
    }

    if (!check_compatible_placements_decisions(ep, extracted_data.objs)) {
      return result;
    }

    if (try_merge_with_previous_table(ep, casted, extracted_data)) {
      // If we merged, then we should ignore this node and keep going,
      // as its information was added to a previous TableLookup instance.

      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::Tofino);

      result.module = new_module;
      result.next_eps.push_back(new_ep);

      return result;
    }

    auto coalesced_data = get_coalescing_data(ep, node);

    auto can_coalesce =
        coalesced_data.can_coalesce &&
        check_compatible_placements_decisions(ep, coalesced_data.get_objs());

    if (can_coalesce) {
      merge_coalesced_data(extracted_data, coalesced_data);
      remember_to_ignore_coalesced(ep, coalesced_data);
    }

    process(ep, node, casted, extracted_data, result);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new TableLookup(node, table_name, objs, keys, params,
                                  contains_symbols, nodes);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const TableLookup *>(other);

    if (table_name != other_cast->get_table_name()) {
      return false;
    }

    const auto &other_objs = other_cast->get_objs();

    if (other_objs.size() != objs.size()) {
      return false;
    }

    for (auto i = 0u; i < objs.size(); i++) {
      auto this_obj = objs[i];
      auto other_obj = other_objs[i];

      if (this_obj != other_obj) {
        return false;
      }
    }

    const auto &other_keys = other_cast->get_keys();

    if (keys.size() != other_keys.size()) {
      return false;
    }

    for (auto i = 0u; i < keys.size(); i++) {
      if (!kutil::solver_toolbox.are_exprs_always_equal(keys[i].expr,
                                                        other_keys[i].expr)) {
        return false;
      }
    }

    const auto &other_params = other_cast->get_params();

    if (params.size() != other_params.size()) {
      return false;
    }

    for (auto i = 0u; i < params.size(); i++) {
      if (params[i].exprs.size() != other_params[i].exprs.size()) {
        return false;
      }

      for (auto j = 0u; j < params[i].exprs.size(); j++) {
        if (!kutil::solver_toolbox.are_exprs_always_equal(
                params[i].exprs[j], other_params[i].exprs[j])) {
          return false;
        }
      }
    }

    const auto &other_contains_symbols = other_cast->get_contains_symbols();

    if (contains_symbols.size() != other_contains_symbols.size()) {
      return false;
    }

    for (auto i = 0u; i < contains_symbols.size(); i++) {
      if (contains_symbols[i].label != other_contains_symbols[i].label) {
        return false;
      }
    }

    const auto &other_nodes = other_cast->get_nodes();

    if (nodes != other_nodes) {
      return false;
    }

    return true;
  }

  const std::string &get_table_name() const { return table_name; }
  const std::vector<obj_addr_t> &get_objs() const { return objs; }
  const std::vector<key_t> &get_keys() const { return keys; }
  const std::vector<param_t> &get_params() const { return params; }
  const std::vector<BDD::symbol_t> &get_contains_symbols() const {
    return contains_symbols;
  }
  const std::unordered_set<BDD::node_id_t> &get_nodes() const { return nodes; }

  void append_to_name(const std::unordered_set<BDD::node_id_t>& node_ids) {
    for (auto node_id : node_ids) {
      table_name += "_" + std::to_string(node_id);
    }
  }

  void add_contains_symbols(const std::vector<BDD::symbol_t> &other_symbols) {
    contains_symbols.insert(contains_symbols.end(), other_symbols.begin(),
                            other_symbols.end());
  }

  void add_nodes(const std::unordered_set<BDD::node_id_t> other_nodes) {
    nodes.insert(other_nodes.begin(), other_nodes.end());
  }

  void add_exprs_to_params(const std::vector<param_t> &other_params) {
    // Careful: we actually want to add expression to each param,
    // not increase the number of params overall

    for (auto i = 0u; i < other_params.size() && i < params.size(); i++) {
      auto &exprs = params[i].exprs;
      const auto &other_exprs = other_params[i].exprs;
      exprs.insert(exprs.end(), other_exprs.begin(), other_exprs.end());
    }
  }
}; // namespace tofino
} // namespace tofino
} // namespace targets
} // namespace synapse
