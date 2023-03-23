#pragma once

#include "klee-util.h"
#include <klee/Expr.h>

#include <string>
#include <vector>

namespace synapse {
namespace synthesizer {
namespace x86_tofino {
class x86TofinoGenerator;

class Transpiler {
private:
  x86TofinoGenerator &tg;

public:
  Transpiler(x86TofinoGenerator &_tg) : tg(_tg) {}

  std::string transpile(const klee::ref<klee::Expr> &expr);
  std::string size_to_type(bits_t size) const;
};
} // namespace x86_tofino
} // namespace synthesizer
} // namespace synapse