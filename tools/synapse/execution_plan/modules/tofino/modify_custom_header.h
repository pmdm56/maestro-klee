#pragma once

#include "ignore.h"
#include "tofino_module.h"

namespace synapse {
namespace targets {
namespace tofino {

class ModifyCustomHeader : public TofinoModule {
private:
  klee::ref<klee::Expr> original_chunk;
  std::vector<modification_t> modifications;

public:
  ModifyCustomHeader()
      : TofinoModule(ModuleType::Tofino_ModifyCustomHeader,
                     "ModifyCustomHeader") {}

  ModifyCustomHeader(BDD::Node_ptr node,
                     const klee::ref<klee::Expr> &_original_chunk,
                     const std::vector<modification_t> &_modifications)
      : TofinoModule(ModuleType::Tofino_ModifyCustomHeader,
                     "ModifyCustomHeader", node),
        original_chunk(_original_chunk), modifications(_modifications) {}

private:
  klee::ref<klee::Expr> get_original_chunk(const ExecutionPlan &ep,
                                           BDD::Node_ptr node) const {
    auto prev_borrows = get_prev_fn(ep, node, symbex::FN_BORROW_CHUNK);
    auto prev_returns = get_prev_fn(ep, node, symbex::FN_RETURN_CHUNK);

    assert(prev_borrows.size());
    assert(prev_borrows.size() > prev_returns.size());

    auto target = prev_borrows[prev_returns.size()];

    auto call_node = BDD::cast_node<BDD::Call>(target);
    assert(call_node);

    auto call = call_node->get_call();

    assert(call.function_name == symbex::FN_BORROW_CHUNK);
    assert(!call.extra_vars[symbex::FN_BORROW_CHUNK_EXTRA].second.isNull());

    return call.extra_vars[symbex::FN_BORROW_CHUNK_EXTRA].second;
  }

  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name != symbex::FN_RETURN_CHUNK) {
      return result;
    }

    assert(!call.args[symbex::FN_BORROW_CHUNK_EXTRA].in.isNull());

    auto _current_chunk = call.args[symbex::FN_BORROW_CHUNK_EXTRA].in;
    auto _original_chunk = get_original_chunk(ep, node);

    auto _modifications = build_modifications(_original_chunk, _current_chunk);
    _modifications = ignore_checksum_modifications(_modifications);

    if (_modifications.size() == 0) {
      auto new_module = std::make_shared<Ignore>(node);
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::Tofino);

      result.module = new_module;
      result.next_eps.push_back(new_ep);

      return result;
    }

    auto new_module = std::make_shared<ModifyCustomHeader>(
        node, _original_chunk, _modifications);
    auto new_ep = ep.add_leaves(new_module, node->get_next());

    result.module = new_module;
    result.next_eps.push_back(new_ep);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const override {
    visitor.visit(ep_node, this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new ModifyCustomHeader(node, original_chunk, modifications);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const ModifyCustomHeader *>(other);

    if (!kutil::solver_toolbox.are_exprs_always_equal(
            original_chunk, other_cast->original_chunk)) {
      return false;
    }

    auto other_modifications = other_cast->get_modifications();

    if (modifications.size() != other_modifications.size()) {
      return false;
    }

    for (unsigned i = 0; i < modifications.size(); i++) {
      auto modification = modifications[i];
      auto other_modification = other_modifications[i];

      if (modification.byte != other_modification.byte) {
        return false;
      }

      if (!kutil::solver_toolbox.are_exprs_always_equal(
              modification.expr, other_modification.expr)) {
        return false;
      }
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_original_chunk() const {
    return original_chunk;
  }

  const std::vector<modification_t> &get_modifications() const {
    return modifications;
  }
};
} // namespace tofino
} // namespace targets
} // namespace synapse
