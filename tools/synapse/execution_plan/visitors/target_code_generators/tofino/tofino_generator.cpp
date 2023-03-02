#include "tofino_generator.h"

#include "../../../../log.h"
#include "../../../../modules/modules.h"
#include "../util.h"

#include "transpiler.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string TofinoGenerator::transpile(klee::ref<klee::Expr> expr) const {
  return tofino::transpile(*this, expr);
}

const tofino::variable_t *
TofinoGenerator::search_variable(std::string symbol) const {
  return ingress.search_variable(symbol);
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
  ingress.synthesize(ingress_apply_code);

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
    if (ep_node->get_module()->get_type() == Module::ModuleType::Tofino_If &&
        pending_packet_borrow_ep &&
        !pending_packet_borrow_next_chunk(branch.get(),
                                          synapse::Target::Tofino)) {
      ingress_parser.transition_reject();
    }

    branch->visit(*this);
  }
}

void TofinoGenerator::visit(const targets::tofino::If *node) {
  auto condition = node->get_condition();
  auto condition_transpiled = transpile(condition);

  ingress.synthesizer.indent();
  ingress.synthesizer.append("if (");
  ingress.synthesizer.append(condition_transpiled);
  ingress.synthesizer.append(") {");
  ingress.synthesizer.append_new_line();

  ingress.synthesizer.inc_indenation();
  ingress.push_pending_if();
}

void TofinoGenerator::visit(const targets::tofino::Then *node) {}

void TofinoGenerator::visit(const targets::tofino::Else *node) {
  ingress.synthesizer.indent();
  ingress.synthesizer.append("else {");
  ingress.synthesizer.append_new_line();
  ingress.synthesizer.inc_indenation();
}

void TofinoGenerator::visit(const targets::tofino::Forward *node) {
  auto port = node->get_port();

  ingress.synthesizer.indent();
  ingress.synthesizer.append(INGRESS_FORWARD_ACTION);
  ingress.synthesizer.append("(");
  ingress.synthesizer.append(port);
  ingress.synthesizer.append(");");
  ingress.synthesizer.append_new_line();

  ingress.close_pending_ifs();
}

void TofinoGenerator::visit(const targets::tofino::EthernetConsume *node) {
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

} // namespace tofino
} // namespace synthesizer
}; // namespace synapse
