#include "tofino_generator.h"

#include "../../../../log.h"
#include "../../../../modules/modules.h"

#define MARKER_INGRESS_METADATA "INGRESS METADATA"
#define MARKER_INGRESS_HEADERS "INGRESS HEADERS"
#define MARKER_EGRESS_METADATA "EGRESS METADATA"
#define MARKER_EGRESS_HEADERS "EGRESS HEADERS"
#define MARKER_INGRESS_PARSE_HEADERS "INGRESS PARSE HEADERS"
#define MARKER_INGRESS_STATE "INGRESS STATE"
#define MARKER_INGRESS_APPLY "INGRESS APPLY"

namespace synapse {
namespace synthesizer {

void TofinoGenerator::visit(ExecutionPlan ep) {
  ExecutionPlanVisitor::visit(ep);

  code_builder.fill_mark(MARKER_INGRESS_METADATA, "");
  code_builder.fill_mark(MARKER_INGRESS_HEADERS, "");
  code_builder.fill_mark(MARKER_EGRESS_METADATA, "");
  code_builder.fill_mark(MARKER_EGRESS_HEADERS, "");

  code_builder.fill_mark(MARKER_INGRESS_PARSE_HEADERS, "state parse_headers {\n"
                                                       "  transition accept;\n"
                                                       "}\n");

  code_builder.fill_mark(MARKER_INGRESS_STATE, "");
  code_builder.fill_mark(MARKER_INGRESS_APPLY, "");
}

void TofinoGenerator::visit(const ExecutionPlanNode *ep_node) {
  auto mod = ep_node->get_module();
  auto next = ep_node->get_next();

  mod->visit(*this);
}

void TofinoGenerator::visit(const targets::tofino::If *node) {
  auto condition = node->get_condition();
  auto condition_transpiled = tofino::transpile(condition);

  std::cerr << "condition  " << util::expr_to_string(condition, true) << "\n";
  std::cerr << "transpiled " << condition_transpiled << "\n";

  ingress_parser.synthesizer.indent();

  ingress_parser.synthesizer.append("if (");
  ingress_parser.synthesizer.append(condition_transpiled);
  ingress_parser.synthesizer.append(") {");

  ingress_parser.synthesizer.inc_indenation();
}

void TofinoGenerator::visit(const targets::tofino::Then *node) {
  assert(false && "TODO");
}

void TofinoGenerator::visit(const targets::tofino::Else *node) {
  assert(false && "TODO");
}

void TofinoGenerator::visit(const targets::tofino::Forward *node) {
  assert(false && "TODO");
}

void TofinoGenerator::visit(const targets::tofino::EthernetConsume *node) {
  assert(false && "TODO");
}

} // namespace synthesizer
}; // namespace synapse
