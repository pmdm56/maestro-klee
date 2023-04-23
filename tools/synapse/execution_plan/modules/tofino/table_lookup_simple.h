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
