#pragma once

#include "../internals.h"

#include "../data_structures/map.h"

namespace BDD {
namespace emulation {

inline void __map_get(const Call *call_node, pkt_t &pkt, time_ns_t time,
                      state_t &state, context_t &ctx, const cfg_t &cfg) {
  auto call = call_node->get_call();

  assert(!call.args[symbex::FN_MAP_ARG_MAP].expr.isNull());
  assert(!call.args[symbex::FN_MAP_ARG_KEY].in.isNull());
  assert(!call.args[symbex::FN_MAP_ARG_OUT].out.isNull());

  auto addr_expr = call.args[symbex::FN_MAP_ARG_MAP].expr;
  auto key_expr = call.args[symbex::FN_MAP_ARG_KEY].in;
  auto value_out_expr = call.args[symbex::FN_MAP_ARG_OUT].out;

  auto generated_symbols = call_node->get_local_generated_symbols();
  auto map_has_this_key_symbol =
      get_symbol(generated_symbols, symbex::MAP_HAS_THIS_KEY);

  auto map_has_this_key = kutil::solver_toolbox.create_new_symbol(
      map_has_this_key_symbol.label, 32);

  auto addr = kutil::expr_addr_to_obj_addr(addr_expr);

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

  int value;
  auto contains = map->get(key, value);

  if (contains) {
    concretize(ctx, value_out_expr, value);
  }

  concretize(ctx, map_has_this_key, contains);
}

inline std::pair<std::string, operation_ptr> map_get() {
  return {symbex::FN_MAP_GET, __map_get};
}

} // namespace emulation
} // namespace BDD