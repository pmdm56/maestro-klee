#pragma once

#include "klee/util/ExprVisitor.h"
#include "load-call-paths.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

class TofinoGenerator;

class Transpiler {
private:
  TofinoGenerator &tg;

public:
  Transpiler(TofinoGenerator &_tg) : tg(_tg) {}

  std::string transpile(const klee::ref<klee::Expr> &expr);
  std::vector<std::string> assign_key_bytes(klee::ref<klee::Expr> key,
                                            const std::vector<meta_t> &meta);
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse