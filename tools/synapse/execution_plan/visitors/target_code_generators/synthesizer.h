#pragma once

#include <assert.h>

#include <sstream>

namespace synapse {
namespace synthesizer {

class Synthesizer {
private:
  std::stringstream ss;
  int lvl;

public:
  Synthesizer() : lvl(0) {}
  Synthesizer(int _lvl) : lvl(_lvl) {}

  void indent() {
    assert(lvl >= 0);
    ss << std::string(lvl * 2, ' ');
  }

  void inc_indenation() {
    assert(lvl >= 0);
    lvl++;
  }

  void dec_indenation() {
    assert(lvl > 0);
    lvl--;
  }

  void append(const std::string code) { ss << code; }
  void append(int n) { ss << n; }

  void append_new_line() { ss << "\n"; }

  std::string dump() const { return ss.str(); }
  void dump(std::ostream &os) const { os << ss.str(); }
};

} // namespace synthesizer
} // namespace synapse