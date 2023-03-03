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

  auto hdr_field = ingress_headers.get_hdr_field_from_chunk(expr);

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

Variable TofinoGenerator::allocate_key_byte(int byte) {
  return ingress.allocate_key_byte(byte);
}

std::vector<Variable>
TofinoGenerator::allocate_key_bytes(klee::ref<klee::Expr> expr) {
  std::vector<Variable> key_bytes;
  auto size_bits = expr->getWidth();

  for (auto byte = 0u; byte * 8 < size_bits; byte++) {
    auto key_byte =
        kutil::solver_toolbox.exprBuilder->Extract(expr, byte * 8, 8);

    auto key_byte_var = allocate_key_byte(byte);
    key_bytes.push_back(key_byte_var);
  }

  return key_bytes;
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

  headers_definitions.synthesize(headers_definitions_code);
  ingress_headers.synthesize(ingress_headers_code);
  ingress_parser.synthesize(ingress_parse_headers_code);
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

  mod->visit(*this);

  auto pending_packet_borrow_ep =
      pending_packet_borrow_next_chunk(ep_node, synapse::Target::Tofino);

  auto parsing_headers = ingress_headers.is_parsing_active();

  if (parsing_headers && !pending_packet_borrow_ep) {
    ingress_parser.transition_accept();
    ingress_headers.deactivate_parsing();
  }

  for (auto branch : next) {
    auto pending_chunk_borrowing_now =
        pending_packet_borrow_next_chunk(branch.get(), synapse::Target::Tofino);

    auto is_conditional =
        ep_node->get_module()->get_type() == Module::ModuleType::Tofino_If;

    if (is_conditional && pending_packet_borrow_ep &&
        !pending_chunk_borrowing_now) {
      ingress_parser.transition_reject();
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

  ingress_parser.add_condition(condition_transpiled);
  ingress.local_vars.push();
  ingress.push_pending_if();
}

void TofinoGenerator::visit(const targets::tofino::Then *node) {
  assert(node->get_node());
  ingress_parser.push_on_true();
}

void TofinoGenerator::visit(const targets::tofino::Else *node) {
  assert(node->get_node());

  ingress_parser.push_on_false();
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
    ingress_parser.pop();
    ingress.local_vars.pop();
  }
}

void TofinoGenerator::visit(const targets::tofino::EthernetConsume *node) {
  assert(node->get_node());
  assert(ingress_headers.is_parsing_active());

  std::vector<header_field_t> fields = {eth_dst_addr, eth_src_addr,
                                        eth_ether_type};

  auto chunk = node->get_chunk();
  auto label = HDR_ETH;
  auto header = header_t(label, chunk, fields);

  headers_definitions.add(header);
  ingress_headers.add(header);
  ingress_parser.add_extractor(label);
}

void TofinoGenerator::visit(const targets::tofino::EthernetModify *node) {
  assert(node->get_node());

  // auto ethernet_chunk = node->get_ethernet_chunk();
  // auto modifications = node->get_modifications();

  // auto field = std::string();
  // auto offset = 0u;

  // for (auto mod : modifications) {
  //   auto byte = mod.byte;
  //   auto expr = mod.expr;

  //   auto modified_byte = kutil::solver_toolbox.exprBuilder->Extract(
  //       ethernet_chunk, byte * 8, klee::Expr::Int8);

  //   field_header_from_packet_chunk(modified_byte, field, offset);

  //   ingress.synthesizer.indent();

  //   ingress.synthesizer.append(field);
  //   ingress.synthesizer.append(" = ");
  //   ingress.synthesizer.append(field);
  //   ingress.synthesizer.append(" & ");
  //   ingress.synthesizer.append("(");

  //   pad(ingress.apply_block, ingress.lvl);
  //   ingress.apply_block << field << " = ";
  //   ingress.apply_block << field << " & ";
  //   ingress.apply_block << "(";
  //   ingress.apply_block << "(";
  //   ingress.apply_block << "(";
  //   auto str = transpile(expr);
  //   str.erase(1, 9); // remove bit<8>...
  //   ingress.apply_block << str;
  //   ingress.apply_block << ")";
  //   ingress.apply_block << " << ";
  //   ingress.apply_block << offset;
  //   ingress.apply_block << ")";
  //   ingress.apply_block << " | ";

  //   uint64_t mask = 0;
  //   for (auto bit = 0u; bit < offset; bit++) {
  //     mask = (mask << 1) | 1;
  //   }

  //   ingress.apply_block << mask;
  //   ingress.apply_block << ")";

  //   ingress.apply_block << ";\n";

  //   assert(field.size());
  // }
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

  std::vector<std::string> keys_bytes_labels;

  for (auto key : keys) {
    auto key_bytes = allocate_key_bytes(key.expr);

    for (auto kb : key_bytes) {
      keys_bytes_labels.push_back(kb.get_label());
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

  table_t table(table_label, keys_bytes_labels, meta_params);
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
    ingress_parser.pop();
    ingress.local_vars.pop();
  }
}

} // namespace tofino
} // namespace synthesizer
}; // namespace synapse
