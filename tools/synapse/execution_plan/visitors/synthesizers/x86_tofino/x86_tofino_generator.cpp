#include "x86_tofino_generator.h"
#include "klee-util.h"

#include "../../../../log.h"
#include "../../../modules/modules.h"
#include "../util.h"
#include "transpiler.h"

#include <sstream>

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

  auto local_var = local_vars.get(symbol);

  if (local_var.valid) {
    return local_var;
  }

  return variable_query_t();
}

variable_query_t
x86TofinoGenerator::search_variable(klee::ref<klee::Expr> expr) const {
  auto local_var = local_vars.get(expr);

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

void x86TofinoGenerator::visit(ExecutionPlan ep) {
  ExecutionPlanVisitor::visit(ep);

  std::stringstream state_decl_code;
  std::stringstream state_init_code;
  std::stringstream nf_process_code;

  state_decl_builder.dump(state_decl_code);
  state_init_builder.dump(state_init_code);
  nf_proces_builder.dump(nf_process_code);

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

void x86TofinoGenerator::visit(const targets::x86_tofino::CurrentTime *node) {
  assert(false && "TODO");
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::PacketParseCPU *node) {
  assert(false && "TODO");
}

void x86TofinoGenerator::visit(const targets::x86_tofino::Drop *node) {
  assert(node);

  nf_proces_builder.indent();
  nf_proces_builder.append("return ");
  nf_proces_builder.append(DROP_PORT_VALUE);
  nf_proces_builder.append(";");
  nf_proces_builder.append_new_line();

  auto closed = pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    local_vars.pop();
  }
}

void x86TofinoGenerator::visit(
    const targets::x86_tofino::ForwardThroughTofino *node) {
  assert(node);
  auto port = node->get_port();

  nf_proces_builder.indent();
  nf_proces_builder.append("return ");
  nf_proces_builder.append(port);
  nf_proces_builder.append(";");
  nf_proces_builder.append_new_line();

  auto closed = pending_ifs.close();

  for (auto i = 0; i < closed; i++) {
    local_vars.pop();
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

  nf_proces_builder.indent();
  nf_proces_builder.append("auto ");
  nf_proces_builder.append(HDR_ETH_VARIABLE);
  nf_proces_builder.append(" = ");
  nf_proces_builder.append(PACKET_VAR_LABEL);
  nf_proces_builder.append(".parse_ethernet();");
  nf_proces_builder.append_new_line();
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

    nf_proces_builder.indent();
    nf_proces_builder.append(transpiled_byte);
    nf_proces_builder.append(" = ");
    nf_proces_builder.append(transpiled_expr);
    nf_proces_builder.append(";");
    nf_proces_builder.append_new_line();
  }
}

void x86TofinoGenerator::visit(const targets::x86_tofino::If *node) {
  assert(node);

  auto condition = node->get_condition();
  auto condition_transpiled = transpile(condition);

  nf_proces_builder.indent();
  nf_proces_builder.append("if (");
  nf_proces_builder.append(condition_transpiled);
  nf_proces_builder.append(") {");
  nf_proces_builder.append_new_line();

  nf_proces_builder.inc_indentation();

  local_vars.push();
  pending_ifs.push();
}

void x86TofinoGenerator::visit(const targets::x86_tofino::Then *node) {
  assert(node);
}

void x86TofinoGenerator::visit(const targets::x86_tofino::Else *node) {
  assert(node);

  local_vars.push();

  nf_proces_builder.indent();
  nf_proces_builder.append("else {");
  nf_proces_builder.append_new_line();
  nf_proces_builder.inc_indentation();
}

} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse