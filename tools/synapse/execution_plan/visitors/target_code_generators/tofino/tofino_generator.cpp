#include "tofino_generator.h"

#include "../../../../log.h"
#include "../../../../modules/modules.h"

#include "transpiler.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

constexpr char MARKER_INGRESS_METADATA[] = "INGRESS METADATA";
constexpr char MARKER_INGRESS_HEADERS[] = "INGRESS HEADERS";
constexpr char MARKER_EGRESS_METADATA[] = "EGRESS METADATA";
constexpr char MARKER_EGRESS_HEADERS[] = "EGRESS HEADERS";
constexpr char MARKER_INGRESS_PARSE_HEADERS[] = "INGRESS PARSE HEADERS";
constexpr char MARKER_INGRESS_STATE[] = "INGRESS STATE";
constexpr char MARKER_INGRESS_APPLY[] = "INGRESS APPLY";

constexpr char INGRESS_FORWARD_ACTION[] = "fwd";
constexpr char INGRESS_DROP_ACTION[] = "drop";
constexpr char INGRESS_FLOOD_ACTION[] = "flood";
constexpr char INGRESS_SEND_TO_CPU_ACTION[] = "send_to_cpu";

std::string TofinoGenerator::transpile(klee::ref<klee::Expr> expr) const {
  return tofino::transpile(*this, expr);
}

const tofino::variable_t *
TofinoGenerator::search_variable(std::string symbol) const {
  return ingress.search_variable(symbol);
}

void TofinoGenerator::visit(ExecutionPlan ep) {
  ExecutionPlanVisitor::visit(ep);

  std::stringstream ingress_parser_code;
  std::stringstream ingress_apply_code;

  ingress_parser.synthesizer.dump(ingress_parser_code);
  ingress.synthesizer.dump(ingress_apply_code);

  code_builder.fill_mark(MARKER_INGRESS_METADATA, "");
  code_builder.fill_mark(MARKER_INGRESS_HEADERS, "");
  code_builder.fill_mark(MARKER_EGRESS_METADATA, "");
  code_builder.fill_mark(MARKER_EGRESS_HEADERS, "");

  code_builder.fill_mark(MARKER_INGRESS_PARSE_HEADERS, "state parse_headers {\n"
                                                       "  transition accept;\n"
                                                       "}\n");

  code_builder.fill_mark(MARKER_INGRESS_STATE, "");
  code_builder.fill_mark(MARKER_INGRESS_APPLY, ingress_apply_code.str());
}

void TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  auto mod = ep_node->get_module();
  auto next = ep_node->get_next();

  mod->visit(*this);

  for (auto branch : next) {
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
  assert(false && "TODO");
}

} // namespace tofino
} // namespace synthesizer
}; // namespace synapse
