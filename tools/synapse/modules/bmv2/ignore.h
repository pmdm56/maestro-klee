#pragma once

#include "../../log.h"
#include "../module.h"
#include "call-paths-to-bdd.h"

namespace synapse {
namespace targets {
namespace bmv2 {

class Ignore : public Module {
private:
  std::vector<std::string> functions_to_ignore;

public:
  Ignore()
      : Module(ModuleType::BMv2_Ignore,
               Target::BMv2, "Ignore") {
    functions_to_ignore =
        std::vector<std::string>{ "current_time", "rte_ether_addr_hash",
                                  "dchain_rejuvenate_index" };
  }

  Ignore(BDD::BDDNode_ptr node)
      : Module(ModuleType::BMv2_Ignore,
               Target::BMv2, "Ignore", node) {}

private:
  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    auto found_it = std::find(functions_to_ignore.begin(),
                              functions_to_ignore.end(), call.function_name);

    if (found_it != functions_to_ignore.end()) {
      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep =
          ep.ignore_leaf(node->get_next(), Target::BMv2);

      result.module = new_module;
      result.next_eps.push_back(new_ep);
    }

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new Ignore(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace bmv2
} // namespace targets
} // namespace synapse
