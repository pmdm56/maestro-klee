#pragma once

#include "klee/Expr.h"

#include <string>
#include <vector>

namespace synapse {
namespace synthesizer {
namespace tofino {

std::string p4_type_from_expr(klee::ref<klee::Expr> expr);

} // namespace tofino
} // namespace synthesizer
} // namespace synapse