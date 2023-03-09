#include "tofino_generator.h"

#include "../../../../log.h"
#include "../../../../modules/modules.h"
#include "../util.h"
#include "klee-util.h"
#include "transpiler.h"
#include "util.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string TofinoGenerator::transpile(klee::ref<klee::Expr> expr) {
  return transpiler.transpile(expr);
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

  auto hdr_field = ingress.headers.get_hdr_field_from_chunk(expr);

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

  std::stringstream headers_definitions_code;
  std::stringstream ingress_headers_code;
  std::stringstream ingress_metadata_code;
  std::stringstream egress_headers_code;
  std::stringstream egress_metadata_code;
  std::stringstream ingress_parse_headers_code;
  std::stringstream ingress_state_code;
  std::stringstream ingress_apply_code;

  ingress.headers.synthesize_defs(headers_definitions_code);
  ingress.headers.synthesize_decl(ingress_headers_code);
  ingress.parser.synthesize(ingress_parse_headers_code);
  ingress.synthesize_user_metadata(ingress_metadata_code);
  ingress.synthesize_state(ingress_state_code);
  ingress.synthesize_apply_block(ingress_apply_code);

  code_builder.fill_mark(MARKER_HEADERS_DEFINITIONS,
                         headers_definitions_code.str());

  code_builder.fill_mark(MARKER_INGRESS_HEADERS, ingress_headers_code.str());
  code_builder.fill_mark(MARKER_INGRESS_METADATA, ingress_metadata_code.str());

  code_builder.fill_mark(MARKER_EGRESS_METADATA, egress_headers_code.str());
  code_builder.fill_mark(MARKER_EGRESS_HEADERS, egress_metadata_code.str());

  code_builder.fill_mark(MARKER_INGRESS_PARSE_HEADERS,
                         ingress_parse_headers_code.str());

  code_builder.fill_mark(MARKER_INGRESS_STATE, ingress_state_code.str());
  code_builder.fill_mark(MARKER_INGRESS_APPLY, ingress_apply_code.str());
}

void TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  auto mod = ep_node->get_module();
  auto next = ep_node->get_next();

  log(ep_node);

  auto pending_packet_borrow_ep =
      pending_packet_borrow_next_chunk(ep_node, synapse::Target::Tofino);

  auto parsing = ingress.parser.is_active();

  if (parsing && !pending_packet_borrow_ep) {
    ingress.parser.transition_accept();
    ingress.parser.deactivate();
  }

  mod->visit(*this);

  for (auto branch : next) {
    auto pending_chunk_borrowing_now =
        pending_packet_borrow_next_chunk(branch.get(), synapse::Target::Tofino);

    auto is_conditional =
        ep_node->get_module()->get_type() == Module::ModuleType::Tofino_If;

    if (is_conditional && pending_packet_borrow_ep &&
        !pending_chunk_borrowing_now) {
      ingress.parser.transition_reject();
    }

    branch->visit(*this);
  }
}

void TofinoGenerator::visit(const targets::tofino::If *node) {
  assert(node->get_node());

  auto condition = node->get_condition();
  auto condition_transpiled = transpile(condition);

  ingress.apply_block_synthesizer.indent();
  ingress.apply_block_synthesizer.append("if (");
  ingress.apply_block_synthesizer.append(condition_transpiled);
  ingress.apply_block_synthesizer.append(") {");
  ingress.apply_block_synthesizer.append_new_line();

  ingress.apply_block_synthesizer.inc_indentation();

  if (ingress.parser.is_active()) {
    ingress.parser.add_condition(condition_transpiled);
  }

  ingress.local_vars.push();
  ingress.push_pending_if();
}

void TofinoGenerator::visit(const targets::tofino::Then *node) {
  assert(node->get_node());

  if (ingress.parser.is_active()) {
    ingress.parser.push_on_true();
  }
}

void TofinoGenerator::visit(const targets::tofino::Else *node) {
  assert(node->get_node());

  if (ingress.parser.is_active()) {
    ingress.parser.push_on_false();
  }

  ingress.local_vars.push();

  ingress.apply_block_synthesizer.indent();
  ingress.apply_block_synthesizer.append("else {");
  ingress.apply_block_synthesizer.append_new_line();
  ingress.apply_block_synthesizer.inc_indentation();
}

void TofinoGenerator::visit(const targets::tofino::Forward *node) {
  assert(node->get_node());
  auto port = node->get_port();

  ingress.apply_block_synthesizer.indent();
  ingress.apply_block_synthesizer.append(INGRESS_FORWARD_ACTION);
  ingress.apply_block_synthesizer.append("(");
  ingress.apply_block_synthesizer.append(port);
  ingress.apply_block_synthesizer.append(");");
  ingress.apply_block_synthesizer.append_new_line();

  auto closed = ingress.close_pending_ifs();

  for (auto i = 0; i < closed; i++) {
    if (ingress.parser.is_active()) {
      ingress.parser.pop();
    }

    ingress.local_vars.pop();
  }
}

void TofinoGenerator::visit(const targets::tofino::EthernetConsume *node) {
  assert(node->get_node());
  assert(ingress.parser.is_active());

  const hdr_field_t eth_dst_addr{DST_ADDR, HDR_ETH_DST_ADDR_FIELD, 48};
  const hdr_field_t eth_src_addr{SRC_ADDR, HDR_ETH_SRC_ADDR_FIELD, 48};
  const hdr_field_t eth_ether_type{ETHER_TYPE, HDR_ETH_ETHER_TYPE_FIELD, 16};

  std::vector<hdr_field_t> fields = {eth_dst_addr, eth_src_addr,
                                     eth_ether_type};

  auto chunk = node->get_chunk();
  auto label = HDR_ETH;
  auto header = Header(ETHERNET, label, chunk, fields);

  ingress.headers.add(header);
  ingress.parser.add_extractor(label);
}

void TofinoGenerator::visit(const targets::tofino::EthernetModify *node) {
  assert(node->get_node());

  auto ethernet_chunk = node->get_ethernet_chunk();
  auto modifications = node->get_modifications();

  for (auto mod : modifications) {
    auto byte = mod.byte;
    auto expr = mod.expr;

    auto modified_byte = kutil::solver_toolbox.exprBuilder->Extract(
        ethernet_chunk, byte * 8, klee::Expr::Int8);
    
    auto transpiled_byte = transpile(modified_byte);
    auto transpiled_expr = transpile(expr);

    ingress.apply_block_synthesizer.indent();
    ingress.apply_block_synthesizer.append(transpiled_byte);
    ingress.apply_block_synthesizer.append(" = ");
    ingress.apply_block_synthesizer.append(transpiled_expr);
    ingress.apply_block_synthesizer.append(";");
    ingress.apply_block_synthesizer.append_new_line();
  }
}

void TofinoGenerator::visit(const targets::tofino::TableLookup *node) {
  assert(node->get_node());

  auto keys = node->get_keys();
  auto params = node->get_params();
  auto bdd_function = node->get_bdd_function();
  auto contains_symbol = node->get_contains_symbol();
  auto table_id = node->get_table_id();

  assert(keys.size());

  std::stringstream table_label_builder;

  table_label_builder << bdd_function;
  table_label_builder << "_";
  table_label_builder << table_id;

  auto table_label = table_label_builder.str();

  std::vector<std::string> key_labels;

  for (auto key : keys) {
    auto key_vars = get_key_vars(ingress, key.expr);

    for (auto kv : key_vars) {
      key_labels.push_back(kv.variable.get_label());
    }
  }

  std::vector<std::vector<std::string>> assignments;

  for (auto key : keys) {
    auto key_assignments = transpiler.assign_key_bytes(key.expr);
    assignments.push_back(key_assignments);
  }

  std::vector<Variable> meta_params;

  for (auto i = 0u; i < params.size(); i++) {
    auto meta_param = ingress.allocate_meta_param(table_label, i, params[i]);
    meta_params.push_back(meta_param);
  }

  table_t table(table_label, key_labels, meta_params);
  ingress.add_table(table);

  assert(keys.size() == assignments.size());

  if (keys.size() == 1) {
    for (auto assignment : assignments[0]) {
      ingress.apply_block_synthesizer.indent();
      ingress.apply_block_synthesizer.append(assignment);
      ingress.apply_block_synthesizer.append(";");
      ingress.apply_block_synthesizer.append_new_line();
    }
  } else {
    for (auto i = 0u; i < keys.size(); i++) {
      auto key = keys[i];
      auto key_assignments = assignments[i];
      auto condition = transpile(key.condition);

      ingress.apply_block_synthesizer.indent();
      ingress.apply_block_synthesizer.append("if (");
      ingress.apply_block_synthesizer.append(condition);
      ingress.apply_block_synthesizer.append(") {");
      ingress.apply_block_synthesizer.append_new_line();

      ingress.apply_block_synthesizer.inc_indentation();

      for (auto assignment : key_assignments) {
        ingress.apply_block_synthesizer.indent();
        ingress.apply_block_synthesizer.append(assignment);
        ingress.apply_block_synthesizer.append(";");
        ingress.apply_block_synthesizer.append_new_line();
      }

      ingress.apply_block_synthesizer.dec_indentation();

      ingress.apply_block_synthesizer.indent();
      ingress.apply_block_synthesizer.append("}");
      ingress.apply_block_synthesizer.append_new_line();
    }
  }

  if (contains_symbol.size()) {
    auto hit_var = Variable(table_label + "_hit", 1, contains_symbol);

    ingress.local_vars.append(hit_var);

    ingress.apply_block_synthesizer.indent();
    ingress.apply_block_synthesizer.append("bool ");
    ingress.apply_block_synthesizer.append(hit_var.get_label());
    ingress.apply_block_synthesizer.append(" = ");
    ingress.apply_block_synthesizer.append(table.label);
    ingress.apply_block_synthesizer.append(".apply().hit;");
    ingress.apply_block_synthesizer.append_new_line();
  } else {
    ingress.apply_block_synthesizer.indent();
    ingress.apply_block_synthesizer.append(table.label);
    ingress.apply_block_synthesizer.append(".apply();");
    ingress.apply_block_synthesizer.append_new_line();
  }
}

void TofinoGenerator::visit(const targets::tofino::Drop *node) {
  ingress.apply_block_synthesizer.indent();
  ingress.apply_block_synthesizer.append("drop();");
  ingress.apply_block_synthesizer.append_new_line();

  auto closed = ingress.close_pending_ifs();

  for (auto i = 0; i < closed; i++) {
    if (ingress.parser.is_active()) {
      ingress.parser.pop();
    }

    ingress.local_vars.pop();
  }
}

} // namespace tofino
} // namespace synthesizer
}; // namespace synapse
