#pragma once

#include "../internals.h"

#include "../data_structures/map.h"

namespace BDD {
namespace emulation {

inline void __map_put(const Call *call_node, pkt_t &pkt, time_ns_t time,
                      state_t &state, context_t &ctx, const cfg_t &cfg) {
  auto call = call_node->get_call();

  assert(!call.args[symbex::FN_MAP_ARG_MAP].expr.isNull());
  assert(!call.args[symbex::FN_MAP_ARG_KEY].in.isNull());
  assert(!call.args[symbex::FN_MAP_ARG_VALUE].expr.isNull());

  auto addr_expr = call.args[symbex::FN_MAP_ARG_MAP].expr;
  auto key_expr = call.args[symbex::FN_MAP_ARG_KEY].in;
  auto value_expr = call.args[symbex::FN_MAP_ARG_VALUE].expr;

  auto addr = kutil::expr_addr_to_obj_addr(addr_expr);
  auto value = kutil::solver_toolbox.value_from_expr(value_expr, ctx);

  auto ds_map = state.get(addr);
  auto map = Map::cast(ds_map);

  auto key_size = key_expr->getWidth();
  auto key = bytes_t(key_size);

  for (auto byte = 0u; byte < key_size / 8; byte++) {
    auto key_byte =
        kutil::solver_toolbox.exprBuilder->Extract(key_expr, byte * 8, 8);
    auto key_byte_value = kutil::solver_toolbox.value_from_expr(key_byte, ctx);
    key[byte] = key_byte_value;
  }

  map->put(key, value);
}

inline std::pair<std::string, operation_ptr> map_put() {
  return {symbex::FN_MAP_PUT, __map_put};
}

} // namespace emulation
} // namespace BDD