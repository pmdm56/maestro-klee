#pragma once

#include "../module.h"
#include "data_structures/int_allocator.h"
#include "ignore.h"
#include "memory_bank.h"

namespace synapse {
namespace targets {
namespace tofino {

class IntegerAllocatorAllocate : public Module {
private:
  IntegerAllocatorRef int_allocator;

public:
  IntegerAllocatorAllocate()
      : Module(ModuleType::Tofino_IntegerAllocatorAllocate, TargetType::Tofino,
               "IntegerAllocatorAllocate") {}

  IntegerAllocatorAllocate(BDD::BDDNode_ptr node,
                           IntegerAllocatorRef _int_allocator)
      : Module(ModuleType::Tofino_IntegerAllocatorAllocate, TargetType::Tofino,
               "IntegerAllocatorAllocate", node),
        int_allocator(_int_allocator) {}

private:
  bool can_place(const ExecutionPlan &ep, obj_addr_t obj,
                 IntegerAllocatorRef &implementation) const {
    auto mb = ep.get_memory_bank<TofinoMemoryBank>(Tofino);

    auto possible = mb->check_implementation_compatibility(
        obj, {
                 DataStructure::Type::INTEGER_ALLOCATOR,
                 DataStructure::Type::TABLE,
             });

    if (!possible) {
      return false;
    }

    auto impls = mb->get_implementations(obj);

    for (auto impl : impls) {
      if (impl->get_type() == DataStructure::INTEGER_ALLOCATOR) {
        implementation = std::dynamic_pointer_cast<IntegerAllocator>(impl);
      }
    }

    return true;
  }

  void save_decision(const ExecutionPlan &ep,
                     DataStructureRef int_allocator) const {
    auto mb = ep.get_memory_bank<TofinoMemoryBank>(Tofino);
    mb->save_implementation(int_allocator);
  }

  processing_result_t process_call(const ExecutionPlan &ep,
                                   BDD::BDDNode_ptr node,
                                   const BDD::Call *casted) override {
    processing_result_t result;
    auto call = casted->get_call();

    if (call.function_name != symbex::FN_DCHAIN_ALLOCATE_NEW_INDEX) {
      return result;
    }

    assert(!call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr.isNull());
    assert(!call.args[symbex::FN_DCHAIN_ARG_TIME].expr.isNull());
    assert(!call.args[symbex::FN_DCHAIN_ARG_OUT].out.isNull());
    assert(!call.ret.isNull());

    auto _dchain = call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr;
    auto _time = call.args[symbex::FN_DCHAIN_ARG_TIME].expr;
    auto _index_out = call.args[symbex::FN_DCHAIN_ARG_OUT].out;
    auto _success = call.ret;

    auto _generated_symbols = casted->get_local_generated_symbols();
    auto _dchain_addr = kutil::expr_addr_to_obj_addr(_dchain);

    auto _out_of_space =
        get_symbol(_generated_symbols, symbex::DCHAIN_OUT_OF_SPACE);

    IntegerAllocatorRef _int_allocator;

    if (!can_place(ep, _dchain_addr, _int_allocator)) {
      return result;
    }

    if (!_int_allocator) {
      auto dchain_config = get_dchain_config(ep.get_bdd(), _dchain_addr);
      auto _capacity = dchain_config.index_range;
      _int_allocator = IntegerAllocator::build(_index_out, _out_of_space, _capacity,
                                               _dchain_addr, {node->get_id()});
    } else {
      _int_allocator->add_integer(_index_out);
      _int_allocator->add_out_of_space(_out_of_space);
      _int_allocator->add_nodes({node->get_id()});
    }

    auto new_module =
        std::make_shared<IntegerAllocatorAllocate>(node, _int_allocator);
    auto new_ep = ep.add_leaves(new_module, node->get_next());

    save_decision(new_ep, _int_allocator);

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new IntegerAllocatorAllocate(node, int_allocator);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const IntegerAllocatorAllocate *>(other);

    return int_allocator->equals(other_cast->get_int_allocator().get());
  }

  IntegerAllocatorRef get_int_allocator() const { return int_allocator; }
};

} // namespace tofino
} // namespace targets
} // namespace synapse
