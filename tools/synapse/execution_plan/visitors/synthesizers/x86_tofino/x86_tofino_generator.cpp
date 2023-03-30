#include "x86_tofino_generator.h"
#include "klee-util.h"

#include "../../../../log.h"
#include "../../../modules/modules.h"
#include "../util.h"
#include "transpiler.h"

#include <sstream>

namespace target = synapse::targets::x86_tofino;

namespace synapse {
namespace synthesizer {
namespace x86_tofino {

std::string x86TofinoGenerator::transpile(klee::ref<klee::Expr> expr) {
  return transpiler.transpile(expr);
}

variable_query_t x86TofinoGenerator::search_variable(std::string symbol) const {
  if (symbol == symbex::PORT || symbol == symbex::PORT2) {
    auto in_port_var = headers.get_hdr_field(CPU, CPU_IN_PORT);

    if (in_port_var.valid) {
      return in_port_var;
    }
  } else if (symbol == symbex::CPU_CODE_PATH) {
    auto code_path_var = headers.get_hdr_field(CPU, CPU_CODE_PATH);

    if (code_path_var.valid) {
      return code_path_var;
    }
  }

  auto local_var = vars.get(symbol);

  if (local_var.valid) {
    return local_var;
  }

  return variable_query_t();
}

variable_query_t
x86TofinoGenerator::search_variable(klee::ref<klee::Expr> expr) const {
  auto local_var = vars.get(expr);

  if (local_var.valid) {
    return local_var;
  }

  auto hdr_field = headers.get_hdr_field_from_chunk(expr);

  if (hdr_field.valid) {
    return hdr_field;
  }

  if (kutil::is_readLSB(expr)) {
    auto symbol = kutil::get_symbol(expr);
    auto variable = search_variable(symbol.second);

    if (variable.valid) {
      return variable;
    }
  }

  return variable_query_t();
}

void x86TofinoGenerator::init_state(ExecutionPlan ep) {
  auto mb = ep.get_memory_bank<target::x86TofinoMemoryBank>(x86_Tofino);
  auto data_structures = mb->get_data_structures();

  for (auto ds : data_structures) {
    switch (ds->type) {
    case target::x86TofinoMemoryBank::ds_type_t::MAP: {
      auto map_ds = static_cast<target::x86TofinoMemoryBank::map_t *>(ds.get());

      auto label = "map_" + std::to_string(map_ds->node_id);
      auto map_var = Variable(label, map_ds->addr);
      auto value_type = transpiler.size_to_type(map_ds->value_size);

      state_decl_builder.indent();
      state_decl_builder.append("std::unique_ptr<Map<");
      state_decl_builder.append(value_type);
      state_decl_builder.append(">> ");
      state_decl_builder.append(label);
      state_decl_builder.append(";");
      state_decl_builder.append_new_line();

      state_init_builder.indent();
      state_init_builder.append(label);
      state_init_builder.append(" = Map<");
      state_init_builder.append(value_type);
      state_init_builder.append(">::build();");
      state_init_builder.append_new_line();

      vars.append(map_var);
    } break;

    case target::x86TofinoMemoryBank::ds_type_t::DCHAIN: {
      auto dchain_ds =
          static_cast<target::x86TofinoMemoryBank::dchain_t *>(ds.get());

      auto label = "dchain_" + std::to_string(dchain_ds->node_id);
      auto dchain_var = Variable(label, dchain_ds->addr);

      state_decl_builder.indent();
      state_decl_builder.append("std::unique_ptr<Dchain> ");
      state_decl_builder.append(label);
      state_decl_builder.append(";");
      state_decl_builder.append_new_line();

      state_init_builder.indent();
      state_init_builder.append(label);
      state_init_builder.append(" = Dchain::build(");
      state_init_builder.append(dchain_ds->index_range);
      state_init_builder.append(");");
      state_init_builder.append_new_line();

      vars.append(dchain_var);
    } break;
    }
  }

  auto time = mb->get_time();
  assert(!time.expr.isNull());

  auto time_var = Variable(TIME_VAR_LABEL, time.expr->getWidth(), {time.label});
  time_var.add_expr(time.expr);
  vars.append(time_var);

  // HACK: we don't care about this symbol
  auto number_of_freed_flows = Variable(symbex::EXPIRE_MAP_FREED_FLOWS, 32,
                                        {symbex::EXPIRE_MAP_FREED_FLOWS});
  vars.append(number_of_freed_flows);

  nf_process_builder.indent();
  nf_process_builder.append(number_of_freed_flows.get_type());
  nf_process_builder.append(" ");
  nf_process_builder.append(number_of_freed_flows.get_label());
  nf_process_builder.append(" = 0");
  nf_process_builder.append(";");
  nf_process_builder.append_new_line();
}

void x86TofinoGenerator::visit(ExecutionPlan ep) {
  init_state(ep);

  ExecutionPlanVisitor::visit(ep);

  std::stringstream state_decl_code;
  std::stringstream state_init_code;
  std::stringstream nf_process_code;

  state_decl_builder.dump(state_decl_code);
  state_init_builder.dump(state_init_code);
  nf_process_builder.dump(nf_process_code);

  fill_mark(MARKER_STATE_DECL, state_decl_code.str());
  fill_mark(MARKER_STATE_INIT, state_init_code.str());
  fill_mark(MARKER_NF_PROCESS, nf_process_code.str());
}

void x86TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  auto mod = ep_node->get_module();
  auto next = ep_node->get_next();

  log(ep_node);

  mod->visit(*this);

  for (auto branch : next) {
    branch->visit(*this);
  }
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::PacketParseCPU *node) {
  assert(false && "TODO");
}

void x86TofinoGenerator::visit(const targets::x86_tofino::Drop *node) {
  assert(node);

  nf_process_builder.indent();
  nf_process_builder.append("return ");
  nf_process_builder.append(DROP_PORT_VALUE);
  nf_process_builder.append(";");
  nf_process_builder.append_new_line();

  auto closed = pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    vars.pop();
  }
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::ForwardThroughTofino *node) {
  assert(node);
  auto port = node->get_port();

  nf_process_builder.indent();
  nf_process_builder.append("return ");
  nf_process_builder.append(port);
  nf_process_builder.append(";");
  nf_process_builder.append_new_line();

  auto closed = pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    vars.pop();
  }
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::PacketParseEthernet *node) {
  assert(node);

  const hdr_field_t eth_dst_addr{ETH_DST_ADDR, HDR_ETH_DST_ADDR_FIELD, 48};
  const hdr_field_t eth_src_addr{ETH_SRC_ADDR, HDR_ETH_SRC_ADDR_FIELD, 48};
  const hdr_field_t eth_ether_type{ETH_ETHER_TYPE, HDR_ETH_ETHER_TYPE_FIELD,
                                   16};

  std::vector<hdr_field_t> fields = {eth_dst_addr, eth_src_addr,
                                     eth_ether_type};

  auto chunk = node->get_chunk();
  auto header = Header(ETHERNET, HDR_ETH_VARIABLE, chunk, fields);

  headers.add(header);

  nf_process_builder.indent();
  nf_process_builder.append("auto ");
  nf_process_builder.append(HDR_ETH_VARIABLE);
  nf_process_builder.append(" = ");
  nf_process_builder.append(PACKET_VAR_LABEL);
  nf_process_builder.append(".parse_ethernet();");
  nf_process_builder.append_new_line();
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::PacketModifyEthernet *node) {
  assert(node);

  auto ethernet_chunk = node->get_ethernet_chunk();
  auto modifications = node->get_modifications();

  for (auto mod : modifications) {
    auto byte = mod.byte;
    auto expr = mod.expr;

    auto modified_byte = kutil::solver_toolbox.exprBuilder->Extract(
        ethernet_chunk, byte * 8, klee::Expr::Int8);

    auto transpiled_byte = transpile(modified_byte);
    auto transpiled_expr = transpile(expr);

    nf_process_builder.indent();
    nf_process_builder.append(transpiled_byte);
    nf_process_builder.append(" = ");
    nf_process_builder.append(transpiled_expr);
    nf_process_builder.append(";");
    nf_process_builder.append_new_line();
  }
}

void x86TofinoGenerator::visit(const targets::x86_tofino::If *node) {
  assert(node);

  auto condition = node->get_condition();
  auto condition_transpiled = transpile(condition);

  nf_process_builder.append_new_line();
  
  nf_process_builder.indent();
  nf_process_builder.append("if (");
  nf_process_builder.append(condition_transpiled);
  nf_process_builder.append(") {");
  nf_process_builder.append_new_line();

  nf_process_builder.inc_indentation();

  vars.push();
  pending_ifs.push();
}

void x86TofinoGenerator::visit(const targets::x86_tofino::Then *node) {
  assert(node);
}

void x86TofinoGenerator::visit(const targets::x86_tofino::Else *node) {
  assert(node);

  vars.push();

  nf_process_builder.indent();
  nf_process_builder.append("else {");
  nf_process_builder.append_new_line();
  nf_process_builder.inc_indentation();
}

void x86TofinoGenerator::visit(const targets::x86_tofino::MapGet *node) {
  assert(node);

  auto map_addr = node->get_map_addr();
  auto key = node->get_key();
  auto map_has_this_key = node->get_map_has_this_key();
  auto value_out = node->get_value_out();

  auto generated_symbols = node->get_generated_symbols();

  assert(!map_addr.isNull());
  assert(!key.isNull());
  assert(!value_out.isNull());

  if (!map_has_this_key.isNull()) {
    auto map_has_this_key_label =
        get_label(generated_symbols, symbex::MAP_HAS_THIS_KEY);
    auto contains_var =
        Variable(map_has_this_key_label, map_has_this_key->getWidth(),
                 {map_has_this_key_label});
    contains_var.add_expr(map_has_this_key);
    vars.append(contains_var);
  }

  auto allocated_index_label = std::string();

  if (has_label(generated_symbols, symbex::MAP_ALLOCATED_INDEX)) {
    allocated_index_label =
        get_label(generated_symbols, symbex::MAP_ALLOCATED_INDEX);
  } else if (has_label(generated_symbols, symbex::VECTOR_VALUE_SYMBOL)) {
    allocated_index_label =
        get_label(generated_symbols, symbex::VECTOR_VALUE_SYMBOL);
  } else {
    assert(false && "No valid generated symbol");
  }

  auto value_var = Variable(allocated_index_label, value_out->getWidth(),
                            {allocated_index_label});
  value_var.add_expr(value_out);
  vars.append(value_var);

  nf_process_builder.append_new_line();

  nf_process_builder.indent();
  nf_process_builder.append(value_var.get_type());
  nf_process_builder.append(" ");
  nf_process_builder.append(value_var.get_label());
  nf_process_builder.append(";");
  nf_process_builder.append_new_line();

  auto map = vars.get(map_addr);

  auto key_label_base = map.var->get_label() + "_key";
  auto key_label = vars.get_new_label(key_label_base);
  auto key_var = Variable(key_label, key->getWidth());

  vars.append(key_var);

  assert(key->getWidth() > 0);
  assert(key->getWidth() % 8 == 0);

  nf_process_builder.indent();
  nf_process_builder.append("bytes_t ");
  nf_process_builder.append(key_label);
  nf_process_builder.append("(");
  nf_process_builder.append(key->getWidth() / 8);
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();

  for (bits_t b = 0u; b < key->getWidth(); b += 8) {
    auto byte = kutil::solver_toolbox.exprBuilder->Extract(key, b, 8);
    auto byte_transpiled = transpile(byte);

    nf_process_builder.indent();
    nf_process_builder.append(key_label);
    nf_process_builder.append("[");
    nf_process_builder.append(b / 8);
    nf_process_builder.append("]");
    nf_process_builder.append(" = ");
    nf_process_builder.append(byte_transpiled);
    nf_process_builder.append(";");
    nf_process_builder.append_new_line();
  }

  nf_process_builder.indent();

  if (!map_has_this_key.isNull()) {
    auto contains_var = vars.get(map_has_this_key);
    assert(contains_var.valid);
    assert(contains_var.offset_bits == 0);
    nf_process_builder.append("auto ");
    nf_process_builder.append(contains_var.var->get_label());
    nf_process_builder.append(" = ");
  }

  nf_process_builder.append("state->");
  nf_process_builder.append(map.var->get_label());
  nf_process_builder.append("->get(");
  nf_process_builder.append(key_label);
  nf_process_builder.append(", ");
  nf_process_builder.append(value_var.get_label());
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();
}

void x86TofinoGenerator::visit(const targets::x86_tofino::MapPut *node) {
  assert(node);

  auto map_addr = node->get_map_addr();
  auto key = node->get_key();
  auto value = node->get_value();

  assert(!map_addr.isNull());
  assert(!key.isNull());
  assert(!value.isNull());

  auto map = vars.get(map_addr);
  assert(map.valid);

  auto key_label_base = map.var->get_label() + "_key";
  auto key_label = vars.get_new_label(key_label_base);
  auto key_var = Variable(key_label, key->getWidth());
  vars.append(key_var);

  auto value_label_base = map.var->get_label() + "_value";
  auto value_label = vars.get_new_label(value_label_base);
  auto value_var = Variable(value_label, value->getWidth());
  vars.append(value_var);

  assert(key->getWidth() > 0);
  assert(key->getWidth() % 8 == 0);

  nf_process_builder.append_new_line();

  nf_process_builder.indent();
  nf_process_builder.append("bytes_t ");
  nf_process_builder.append(key_label);
  nf_process_builder.append("(");
  nf_process_builder.append(key->getWidth() / 8);
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();

  for (bits_t b = 0u; b < key->getWidth(); b += 8) {
    auto byte = kutil::solver_toolbox.exprBuilder->Extract(key, b, 8);
    auto byte_transpiled = transpile(byte);

    nf_process_builder.indent();
    nf_process_builder.append(key_label);
    nf_process_builder.append("[");
    nf_process_builder.append(b / 8);
    nf_process_builder.append("]");
    nf_process_builder.append(" = ");
    nf_process_builder.append(byte_transpiled);
    nf_process_builder.append(";");
    nf_process_builder.append_new_line();
  }

  if (is_primitive_type(value->getWidth())) {
    auto value_transpiled = transpile(value);

    nf_process_builder.indent();
    nf_process_builder.append(value_var.get_type());
    nf_process_builder.append(" ");
    nf_process_builder.append(value_var.get_label());
    nf_process_builder.append(" = ");
    nf_process_builder.append(value_transpiled);
    nf_process_builder.append(";");
    nf_process_builder.append_new_line();
  } else {
    nf_process_builder.indent();
    nf_process_builder.append("bytes_t ");
    nf_process_builder.append(value_var.get_label());
    nf_process_builder.append("(");
    nf_process_builder.append(value->getWidth() / 8);
    nf_process_builder.append(");");
    nf_process_builder.append_new_line();

    for (bits_t b = 0u; b < value->getWidth(); b += 8) {
      auto byte = kutil::solver_toolbox.exprBuilder->Extract(value, b, 8);
      auto byte_transpiled = transpile(byte);

      nf_process_builder.indent();
      nf_process_builder.append(value_var.get_label());
      nf_process_builder.append("[");
      nf_process_builder.append(b / 8);
      nf_process_builder.append("]");
      nf_process_builder.append(" = ");
      nf_process_builder.append(byte_transpiled);
      nf_process_builder.append(";");
      nf_process_builder.append_new_line();
    }
  }

  nf_process_builder.indent();
  nf_process_builder.append("state->");
  nf_process_builder.append(map.var->get_label());
  nf_process_builder.append("->put(");
  nf_process_builder.append(key_label);
  nf_process_builder.append(", ");
  nf_process_builder.append(value_var.get_label());
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();
}

void x86TofinoGenerator::visit(const targets::x86_tofino::EtherAddrHash *node) {
  assert(node);

  auto addr = node->get_addr();
  auto generated_symbol = node->get_generated_symbol();

  auto addr_transpiled = transpile(addr);
  auto hash_var = Variable(generated_symbol.label, generated_symbol.expr);

  vars.append(hash_var);

  nf_process_builder.indent();
  nf_process_builder.append("auto ");
  nf_process_builder.append(hash_var.get_label());
  nf_process_builder.append(" = ");
  nf_process_builder.append("ether_addr_hash(");
  nf_process_builder.append(addr_transpiled);
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::DchainAllocateNewIndex *node) {
  assert(node);

  auto dchain_addr = node->get_dchain_addr();
  auto time = node->get_time();
  auto index_out = node->get_index_out();
  auto success = node->get_success();
  auto generated_symbols = node->get_generated_symbols();

  assert(!dchain_addr.isNull());
  assert(!time.isNull());
  assert(!index_out.isNull());
  assert(!success.isNull());
  assert(generated_symbols.size() == 2);

  auto out_of_space =
      get_symbol(generated_symbols, symbex::DCHAIN_OUT_OF_SPACE);
  auto out_of_space_var =
      Variable(out_of_space.label, success->getWidth(), {out_of_space.label});
  auto out_of_space_type = out_of_space_var.get_type();
  out_of_space_var.add_expr(out_of_space.expr);
  vars.append(out_of_space_var);

  auto new_index = get_symbol(generated_symbols, symbex::DCHAIN_NEW_INDEX);
  auto new_index_var =
      Variable(new_index.label, success->getWidth(), {new_index.label});
  auto new_index_type = new_index_var.get_type();
  new_index_var.add_expr(new_index.expr);
  vars.append(new_index_var);

  auto dchain = vars.get(dchain_addr);
  auto time_transpiled = transpile(time);

  assert(dchain.valid);

  nf_process_builder.indent();
  nf_process_builder.append(new_index_type);
  nf_process_builder.append(" ");
  nf_process_builder.append(new_index_var.get_label());
  nf_process_builder.append(";");
  nf_process_builder.append_new_line();

  nf_process_builder.indent();
  nf_process_builder.append(out_of_space_type);
  nf_process_builder.append(" ");
  nf_process_builder.append(out_of_space_var.get_label());
  nf_process_builder.append(" = ");
  nf_process_builder.append("state->");
  nf_process_builder.append(dchain.var->get_label());
  nf_process_builder.append("->allocate_new_index(");
  nf_process_builder.append(new_index_var.get_label());
  nf_process_builder.append(", ");
  nf_process_builder.append(time_transpiled);
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::DchainIsIndexAllocated *node) {
  assert(false && "TODO");
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::DchainRejuvenateIndex *node) {
  assert(node);

  auto dchain_addr = node->get_dchain_addr();
  auto time = node->get_time();
  auto index = node->get_index();

  assert(!dchain_addr.isNull());
  assert(!time.isNull());
  assert(!index.isNull());

  auto dchain = vars.get(dchain_addr);
  auto index_transpiled = transpile(index);
  auto time_transpiled = transpile(time);

  assert(dchain.valid);

  nf_process_builder.indent();
  nf_process_builder.append("state->");
  nf_process_builder.append(dchain.var->get_label());
  nf_process_builder.append("->rejuvenate_index(");
  nf_process_builder.append(index_transpiled);
  nf_process_builder.append(", ");
  nf_process_builder.append(time_transpiled);
  nf_process_builder.append(");");
  nf_process_builder.append_new_line();
}

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse