#pragma once

#include "x86_module.h"

namespace synapse {
namespace targets {
namespace x86 {

class SketchExpire : public x86Module {
private:
  addr_t sketch_addr;
  klee::ref<klee::Expr> time;

public:
  SketchExpire() : x86Module(ModuleType::x86_SketchExpire, "SketchExpire") {}

  SketchExpire(BDD::Node_ptr node, addr_t _sketch_addr,
               klee::ref<klee::Expr> _time)
      : x86Module(ModuleType::x86_SketchExpire, "SketchExpire", node),
        sketch_addr(_sketch_addr), time(_time) {}

private:
  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;

    auto casted = BDD::cast_node<BDD::Call>(node);

    if (!casted) {
      return result;
    }

    auto call = casted->get_call();

    if (call.function_name != symbex::FN_SKETCH_EXPIRE) {
      return result;
    }

    assert(!call.args[symbex::FN_SKETCH_ARG_SKETCH].expr.isNull());
    assert(!call.args[symbex::FN_SKETCH_ARG_TIME].expr.isNull());

    auto _sketch = call.args[symbex::FN_SKETCH_ARG_SKETCH].expr;
    auto _time = call.args[symbex::FN_SKETCH_ARG_TIME].expr;

    auto _sketch_addr = kutil::expr_addr_to_obj_addr(_sketch);

    save_sketch(ep, _sketch_addr);

    auto new_module = std::make_shared<SketchExpire>(node, _sketch_addr, _time);
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
    auto cloned = new SketchExpire(node, sketch_addr, time);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const SketchExpire *>(other);

    if (sketch_addr != other_cast->get_sketch_addr()) {
      return false;
    }

    if (!kutil::solver_toolbox.are_exprs_always_equal(time,
                                                      other_cast->get_time())) {
      return false;
    }

    return true;
  }

  addr_t get_sketch_addr() const { return sketch_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};
} // namespace x86
} // namespace targets
} // namespace synapse
