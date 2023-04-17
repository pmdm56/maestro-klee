#pragma once

#include "table_lookup.h"

namespace synapse {
namespace targets {
namespace tofino {

class TableLookupSimple : public TableLookup {

public:
  TableLookupSimple()
      : TableLookup(ModuleType::Tofino_TableLookupSimple, "TableLookupSimple") {
  }

  TableLookupSimple(BDD::BDDNode_ptr node, std::string _table_name,
                    const std::unordered_set<obj_addr_t> &_objs,
                    const std::vector<key_t> &_keys,
                    const std::vector<param_t> &_params,
                    const std::vector<BDD::symbol_t> &_contains_symbols,
                    const std::unordered_set<BDD::node_id_t> &_nodes)
      : TableLookup(ModuleType::Tofino_TableLookupSimple, "TableLookupSimple",
                    node, _table_name, _objs, _keys, _params, _contains_symbols,
                    _nodes) {}

private:
  virtual std::string build_table_name(
      const std::unordered_set<BDD::node_id_t> &_node_ids) const override {
    // Use a counter to avoid name collision.

    static int counter = 0;
    std::stringstream table_label_builder;

    table_label_builder << "simple_table";

    for (auto node_id : _node_ids) {
      table_label_builder << "_";
      table_label_builder << node_id;
    }

    table_label_builder << "_";
    table_label_builder << counter;

    counter++;

    return table_label_builder.str();
  }

  bool process(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
               const BDD::Call *casted, const extracted_data_t data,
               processing_result_t &result) {
    auto _table_name = build_table_name(data.node_ids);

    auto new_module = std::make_shared<TableLookupSimple>(
        node, _table_name, data.objs, data.keys, data.values,
        data.contains_symbols, data.node_ids);

    auto new_ep = ep.add_leaves(new_module, node->get_next());

    save_decision(new_ep, data.objs, new_module,
                  PlacementDecision::TofinoTableSimple);

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

    if (!check_compatible_placements_decisions(
            ep, extracted_data.objs, PlacementDecision::TofinoTableSimple)) {
      return result;
    }

    process(ep, node, casted, extracted_data, result);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new TableLookupSimple(node, table_name, objs, keys, params,
                                        contains_symbols, nodes);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    return TableLookup::equals(other);
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse
