#include "symbex.h"

namespace synapse {
namespace symbex {

std::pair<bool, obj_addr_t> get_obj_from_call(const BDD::Call *node_call) {
  std::pair<bool, obj_addr_t> result;

  result.first = false;

  assert(node_call);
  auto call = node_call->get_call();

  klee::ref<klee::Expr> obj;

  if (call.function_name == FN_VECTOR_BORROW ||
      call.function_name == FN_VECTOR_RETURN) {
    assert(!call.args[symbex::FN_VECTOR_ARG_VECTOR].expr.isNull());
    obj = call.args[symbex::FN_VECTOR_ARG_VECTOR].expr;
  }

  else if (call.function_name == FN_MAP_GET ||
           call.function_name == FN_MAP_PUT) {
    assert(!call.args[symbex::FN_MAP_ARG_MAP].expr.isNull());
    obj = call.args[symbex::FN_MAP_ARG_MAP].expr;
  }

  else if (call.function_name == FN_DCHAIN_ALLOCATE_NEW_INDEX ||
           call.function_name == FN_DCHAIN_IS_ALLOCATED ||
           call.function_name == FN_DCHAIN_REJUVENATE) {
    assert(!call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr.isNull());
    obj = call.args[symbex::FN_DCHAIN_ARG_CHAIN].expr;
  }

  if (!obj.isNull()) {
    auto addr = kutil::expr_addr_to_obj_addr(obj);
    result.first = true;
    result.second = addr;
  }

  return result;
}

} // namespace symbex
} // namespace synapse