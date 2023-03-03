#include "util.h"
#include "klee-util.h"
#include "tofino_generator.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string p4_type_from_expr(klee::ref<klee::Expr> expr) {
  auto size_bits = expr->getWidth();
  std::stringstream label;
  label << "bit<" << size_bits << ">";
  return label.str();
}

} // namespace tofino
} // namespace synthesizer
} // namespace synapse