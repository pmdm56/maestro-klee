#include "klee-util.h"

#include "../../../../log.h"
#include "../../../modules/modules.h"
#include "../util.h"
#include "tofino_generator.h"
#include "transpiler.h"
#include "util.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string TofinoGenerator::transpile(klee::ref<klee::Expr> expr) {
  auto code = transpiler.transpile(expr);
  return code;
}

variable_query_t TofinoGenerator::search_variable(std::string symbol) const {
  auto ingress_var = ingress.search_variable(symbol);

  if (ingress_var.valid) {
    return ingress_var;
  }

  return variable_query_t();
}

variable_query_t
TofinoGenerator::search_variable(klee::ref<klee::Expr> expr) const {
  auto ingress_var = ingress.search_variable(expr);

  if (ingress_var.valid) {
    return ingress_var;
  }

  auto hdr_field = ingress.headers.query_hdr_field_from_chunk(expr);

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

void TofinoGenerator::visit(ExecutionPlan ep) {
  ExecutionPlanVisitor::visit(ep);

  std::stringstream ingress_metadata_code;
  std::stringstream egress_metadata_code;
  std::stringstream ingress_state_code;
  std::stringstream ingress_apply_code;

  ingress.synthesize_user_metadata(ingress_metadata_code);
  ingress.synthesize_state(ingress_state_code);
  ingress.synthesize_apply_block(ingress_apply_code);

  fill_mark(MARKER_INGRESS_METADATA, ingress_metadata_code.str());
  fill_mark(MARKER_EGRESS_METADATA, egress_metadata_code.str());
  fill_mark(MARKER_INGRESS_STATE, ingress_state_code.str());
  fill_mark(MARKER_INGRESS_APPLY, ingress_apply_code.str());
}

void TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  auto mod = ep_node->get_module();
  auto next = ep_node->get_next();

  log(ep_node);

  mod->visit(*this);

  for (auto branch : next) {
    branch->visit(*this);
  }
}

// Due to packet byte limitations (max 4 bytes per branching condition)
void TofinoGenerator::visit_if_multiple_conditions(
    std::vector<klee::ref<klee::Expr>> conditions) {
  auto cond = ingress.allocate_local_auxiliary("cond", 1);

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append(cond.get_type());
  ingress.apply_block_builder.append(" ");
  ingress.apply_block_builder.append(cond.get_label());
  ingress.apply_block_builder.append(" = false;");
  ingress.apply_block_builder.append_new_line();

  for (auto c : conditions) {
    auto transpiled = transpile(c);

    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append("if (");
    ingress.apply_block_builder.append(transpiled);
    ingress.apply_block_builder.append(") {");
    ingress.apply_block_builder.append_new_line();

    ingress.apply_block_builder.inc_indentation();
  }

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append(cond.get_label());
  ingress.apply_block_builder.append(" = true;");
  ingress.apply_block_builder.append_new_line();

  for (auto c : conditions) {
    ingress.apply_block_builder.dec_indentation();

    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append("}");
    ingress.apply_block_builder.append_new_line();
  }

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append("if (");
  ingress.apply_block_builder.append(cond.get_label());
  ingress.apply_block_builder.append(") {");
  ingress.apply_block_builder.append_new_line();

  ingress.apply_block_builder.inc_indentation();

  ingress.local_vars.push();
  ingress.pending_ifs.push();
}

void TofinoGenerator::visit_if_simple_condition(
    klee::ref<klee::Expr> condition) {
  auto transpiled = transpile(condition);

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append("if (");
  ingress.apply_block_builder.append(transpiled);
  ingress.apply_block_builder.append(") {");
  ingress.apply_block_builder.append_new_line();

  ingress.apply_block_builder.inc_indentation();

  ingress.local_vars.push();
  ingress.pending_ifs.push();
}

void TofinoGenerator::visit(const targets::tofino::If *node) {
  assert(node);

  auto conditions = node->get_conditions();
  assert(conditions.size() > 0);

  if (conditions.size() > 1) {
    visit_if_multiple_conditions(conditions);
  } else {
    visit_if_simple_condition(conditions[0]);
  }
}

void TofinoGenerator::visit(const targets::tofino::IfHeaderValid *node) {
  assert(node);

  auto header = node->get_header();

  std::stringstream ingress_condition_builder;

  ingress_condition_builder << INGRESS_PACKET_HEADER_VARIABLE;
  ingress_condition_builder << ".";

  switch (header) {
  case targets::tofino::IfHeaderValid::PacketHeader::ETHERNET: {
    ingress_condition_builder << HDR_ETH;
    ingress_condition_builder << ".isValid()";
  } break;
  case targets::tofino::IfHeaderValid::PacketHeader::IPV4: {
    ingress_condition_builder << HDR_IPV4;
    ingress_condition_builder << ".isValid()";
  } break;
  case targets::tofino::IfHeaderValid::PacketHeader::NOT_IPV4_OPTIONS: {
    ingress_condition_builder << HDR_IPV4;
    ingress_condition_builder << ".";
    ingress_condition_builder << HDR_IPV4_IHL_FIELD;
    ingress_condition_builder << " <= 5";
  } break;
  case targets::tofino::IfHeaderValid::PacketHeader::TCPUDP: {
    ingress_condition_builder << HDR_TCPUDP;
    ingress_condition_builder << ".isValid()";
  } break;
  default:
    assert(false && "Should not be here.");
  }

  auto transpiled = ingress_condition_builder.str();

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append("if (");
  ingress.apply_block_builder.append(transpiled);
  ingress.apply_block_builder.append(") {");
  ingress.apply_block_builder.append_new_line();

  ingress.apply_block_builder.inc_indentation();

  ingress.local_vars.push();
  ingress.pending_ifs.push();
}

void TofinoGenerator::visit(const targets::tofino::Then *node) { assert(node); }

void TofinoGenerator::visit(const targets::tofino::Else *node) {
  assert(node);

  ingress.local_vars.push();

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append("else {");
  ingress.apply_block_builder.append_new_line();
  ingress.apply_block_builder.inc_indentation();
}

void TofinoGenerator::visit(const targets::tofino::Forward *node) {
  assert(node);
  auto port = node->get_port();

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append(INGRESS_FORWARD_ACTION);
  ingress.apply_block_builder.append("(");
  ingress.apply_block_builder.append(port);
  ingress.apply_block_builder.append(");");
  ingress.apply_block_builder.append_new_line();

  auto closed = ingress.pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    ingress.local_vars.pop();
  }
}

void TofinoGenerator::visit(const targets::tofino::EthernetConsume *node) {
  assert(node);

  const hdr_field_t eth_dst_addr{ETH_DST_ADDR, HDR_ETH_DST_ADDR_FIELD, 48};
  const hdr_field_t eth_src_addr{ETH_SRC_ADDR, HDR_ETH_SRC_ADDR_FIELD, 48};
  const hdr_field_t eth_ether_type{ETH_ETHER_TYPE, HDR_ETH_ETHER_TYPE_FIELD,
                                   16};

  std::vector<hdr_field_t> fields = {eth_dst_addr, eth_src_addr,
                                     eth_ether_type};

  auto chunk = node->get_chunk();
  auto label = HDR_ETH;
  auto header = Header(ETHERNET, label, chunk, fields);

  ingress.headers.add(header);
}

void TofinoGenerator::visit(const targets::tofino::EthernetModify *node) {
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

    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append(transpiled_byte);
    ingress.apply_block_builder.append(" = ");
    ingress.apply_block_builder.append(transpiled_expr);
    ingress.apply_block_builder.append(";");
    ingress.apply_block_builder.append_new_line();
  }
}

void TofinoGenerator::visit(const targets::tofino::IPv4Consume *node) {
  assert(node);

  const hdr_field_t ver{IPV4_VERSION, HDR_IPV4_VERSION_FIELD, 4};
  const hdr_field_t ihl{IPV4_IHL, HDR_IPV4_IHL_FIELD, 4};
  const hdr_field_t dscp{IPV4_DSCP, HDR_IPV4_DSCP_FIELD, 8};
  const hdr_field_t tot_len{IPV4_TOT_LEN, HDR_IPV4_TOT_LEN_FIELD, 16};
  const hdr_field_t id{IPV4_ID, HDR_IPV4_ID_FIELD, 16};
  const hdr_field_t frag_off{IPV4_FRAG_OFF, HDR_IPV4_FRAG_OFF_FIELD, 16};
  const hdr_field_t ttl{IPV4_TTL, HDR_IPV4_TTL_FIELD, 8};
  const hdr_field_t protocol{IPV4_PROTOCOL, HDR_IPV4_PROTOCOL_FIELD, 8};
  const hdr_field_t check{IPV4_CHECK, HDR_IPV4_CHECK_FIELD, 16};
  const hdr_field_t src_ip{IPV4_SRC_IP, HDR_IPV4_SRC_ADDR_FIELD, 32};
  const hdr_field_t dst_ip{IPV4_DST_IP, HDR_IPV4_DST_ADDR_FIELD, 32};

  std::vector<hdr_field_t> fields = {
      ver, ihl,      dscp,  tot_len, id,     frag_off,
      ttl, protocol, check, src_ip,  dst_ip,
  };

  auto chunk = node->get_chunk();
  auto label = HDR_IPV4;
  auto header = Header(IPV4, label, chunk, fields);

  ingress.headers.add(header);
}

void TofinoGenerator::visit(const targets::tofino::IPv4Modify *node) {
  assert(node);

  auto ipv4_chunk = node->get_ipv4_chunk();
  auto modifications = node->get_modifications();

  for (auto mod : modifications) {
    auto byte = mod.byte;
    auto expr = mod.expr;

    if (byte >= 10 && byte <= 11) {
      // We don't care about changes to the checksum here
      continue;
    }

    auto modified_byte = kutil::solver_toolbox.exprBuilder->Extract(
        ipv4_chunk, byte * 8, klee::Expr::Int8);

    auto transpiled_byte = transpile(modified_byte);
    auto transpiled_expr = transpile(expr);

    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append(transpiled_byte);
    ingress.apply_block_builder.append(" = ");
    ingress.apply_block_builder.append(transpiled_expr);
    ingress.apply_block_builder.append(";");
    ingress.apply_block_builder.append_new_line();
  }
}

void TofinoGenerator::visit(const targets::tofino::IPv4OptionsConsume *node) {
  assert(node);

  auto length = node->get_length();

  const hdr_field_t value{IPV4_OPTIONS_VALUE, HDR_IPV4_OPTIONS_VALUE_FIELD, 320,
                          length};
  std::vector<hdr_field_t> fields = {value};

  auto chunk = node->get_chunk();
  auto label = HDR_IPV4_OPTIONS;
  auto header = Header(IPV4_OPTIONS, label, chunk, fields);

  ingress.headers.add(header);
}

void TofinoGenerator::visit(const targets::tofino::TCPUDPConsume *node) {
  const hdr_field_t src_port{TCPUDP_SRC_PORT, HDR_TCPUDP_SRC_PORT_FIELD, 16};
  const hdr_field_t dst_port{TCPUDP_DST_PORT, HDR_TCPUDP_DST_PORT_FIELD, 16};

  std::vector<hdr_field_t> fields = {
      src_port,
      dst_port,
  };

  auto chunk = node->get_chunk();
  auto label = HDR_TCPUDP;
  auto header = Header(TCPUDP, label, chunk, fields);

  ingress.headers.add(header);
}

void TofinoGenerator::visit(const targets::tofino::TCPUDPModify *node) {
  assert(node);

  auto tcpudp_chunk = node->get_tcpudp_chunk();
  auto modifications = node->get_modifications();

  for (auto mod : modifications) {
    auto byte = mod.byte;
    auto expr = mod.expr;

    auto modified_byte = kutil::solver_toolbox.exprBuilder->Extract(
        tcpudp_chunk, byte * 8, klee::Expr::Int8);

    auto transpiled_byte = transpile(modified_byte);
    auto transpiled_expr = transpile(expr);

    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append(transpiled_byte);
    ingress.apply_block_builder.append(" = ");
    ingress.apply_block_builder.append(transpiled_expr);
    ingress.apply_block_builder.append(";");
    ingress.apply_block_builder.append_new_line();
  }
}

void TofinoGenerator::visit(
    const targets::tofino::IPv4TCPUDPChecksumsUpdate *node) {
  // TODO: implement
}

void TofinoGenerator::visit(const targets::tofino::TableLookup *node) {
  assert(node);

  auto table_name = node->get_table_name();
  auto keys = node->get_keys();
  auto params = node->get_params();
  auto contains_symbols = node->get_contains_symbols();

  assert(keys.size());

  std::vector<std::string> key_labels;
  std::vector<std::vector<std::string>> assignments;

  for (auto key : keys) {
    auto key_vars = get_key_vars(ingress, key.expr, key.meta);

    for (auto kv : key_vars) {
      key_labels.push_back(kv.variable.get_label());
    }

    auto key_assignments = transpiler.assign_key_bytes(key.expr, key_vars);
    assignments.push_back(key_assignments);
  }

  Variables meta_params;

  for (auto i = 0u; i < params.size(); i++) {
    auto meta_param =
        ingress.allocate_meta_param(table_name, i, params[i].exprs);
    meta_params.push_back(meta_param);
  }

  table_t table(table_name, key_labels, meta_params);
  ingress.add_table(table);

  assert(keys.size() == assignments.size());

  if (keys.size() == 1) {
    for (auto assignment : assignments[0]) {
      ingress.apply_block_builder.indent();
      ingress.apply_block_builder.append(assignment);
      ingress.apply_block_builder.append(";");
      ingress.apply_block_builder.append_new_line();
    }
  } else {
    for (auto i = 0u; i < keys.size(); i++) {
      auto key = keys[i];
      auto key_assignments = assignments[i];
      auto condition = transpile(key.condition);

      ingress.apply_block_builder.indent();
      ingress.apply_block_builder.append("if (");
      ingress.apply_block_builder.append(condition);
      ingress.apply_block_builder.append(") {");
      ingress.apply_block_builder.append_new_line();

      ingress.apply_block_builder.inc_indentation();

      for (auto assignment : key_assignments) {
        ingress.apply_block_builder.indent();
        ingress.apply_block_builder.append(assignment);
        ingress.apply_block_builder.append(";");
        ingress.apply_block_builder.append_new_line();
      }

      ingress.apply_block_builder.dec_indentation();

      ingress.apply_block_builder.indent();
      ingress.apply_block_builder.append("}");
      ingress.apply_block_builder.append_new_line();
    }
  }

  if (contains_symbols.size() > 0) {
    std::vector<std::string> symbols_labels;

    for (auto symbol : contains_symbols) {
      symbols_labels.push_back(symbol.label);
    }

    auto hit_var = ingress.allocate_local_auxiliary(table_name + "_hit", 1);
    hit_var.add_symbols(symbols_labels);

    ingress.local_vars.append(hit_var);

    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append("bool ");
    ingress.apply_block_builder.append(hit_var.get_label());
    ingress.apply_block_builder.append(" = ");
    ingress.apply_block_builder.append(table_name);
    ingress.apply_block_builder.append(".apply().hit;");
    ingress.apply_block_builder.append_new_line();
  } else {
    ingress.apply_block_builder.indent();
    ingress.apply_block_builder.append(table_name);
    ingress.apply_block_builder.append(".apply();");
    ingress.apply_block_builder.append_new_line();
  }
}

void TofinoGenerator::visit(const targets::tofino::Drop *node) {
  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append("drop();");
  ingress.apply_block_builder.append_new_line();

  auto closed = ingress.pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    ingress.local_vars.pop();
  }
}

void TofinoGenerator::visit(const targets::tofino::Ignore *node) {
  assert(false && "TODO");
}

void TofinoGenerator::visit(const targets::tofino::SendToController *node) {
  auto cpu_code_path = node->get_cpu_code_path();

  ingress.apply_block_builder.indent();
  ingress.apply_block_builder.append(INGRESS_SEND_TO_CPU_ACTION);
  ingress.apply_block_builder.append("(");
  ingress.apply_block_builder.append(cpu_code_path);
  ingress.apply_block_builder.append(");");
  ingress.apply_block_builder.append_new_line();

  auto closed = ingress.pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    ingress.local_vars.pop();
  }
}

} // namespace tofino
} // namespace synthesizer
}; // namespace synapse
