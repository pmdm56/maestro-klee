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

  TableLookupSimple(BDD::BDDNode_ptr node, TableRef _table)
      : TableLookup(ModuleType::Tofino_TableLookupSimple, "TableLookupSimple",
                    node, _table) {}

private:
  TableRef build_non_mergeable_table(const BDD::Call *casted) const {
    auto data = get_extract_data(casted);

    if (!data.valid) {
      return TableRef();
    }

    auto node_id = casted->get_id();

    std::vector<BDD::symbol_t> hit;

    if (data.hit.first) {
      hit.push_back(data.hit.second);
    }

    auto table = TableNonMergeable::build(data.keys, data.values, hit,
                                          {data.obj}, {node_id});

    return table;
  }

  virtual bool lookup_already_done(const ExecutionPlan &ep,
                                   TableRef target) const override {
    auto prev_modules =
        get_prev_modules(ep, {ModuleType::Tofino_TableLookupSimple});

    for (auto module : prev_modules) {
      auto lookup = static_cast<TableLookupSimple *>(module.get());
      auto table = lookup->get_table();

      if (table->equals(target.get())) {
        return true;
      }
    }

    return false;
  }

  virtual bool check_compatible_placements_decisions(
      const ExecutionPlan &ep,
      const std::unordered_set<obj_addr_t> &objs) const override {
    auto mb = ep.get_memory_bank<TofinoMemoryBank>(Tofino);

    for (auto obj : objs) {
      auto compatible = mb->check_implementation_compatibility(
          obj, {DataStructure::Type::TABLE_NON_MERGEABLE});

      if (!compatible) {
        return false;
      }
    }

    return true;
  }

  virtual bool process(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
                       const BDD::Call *casted, TableRef table,
                       processing_result_t &result) override {
    auto new_module = std::make_shared<TableLookupSimple>(node, table);
    auto new_ep = ep.add_leaves(new_module, node->get_next());

    save_decision(new_ep, table);

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return true;
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;

    auto table = build_non_mergeable_table(casted);

    if (!table) {
      return result;
    }

    if (!check_compatible_placements_decisions(ep, table->get_objs())) {
      return result;
    }

    process(ep, node, casted, table, result);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new TableLookupSimple(node, table);
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
