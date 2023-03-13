#pragma once

#include <memory>
#include <string>

namespace synapse {
namespace synthesizer {
namespace tofino {

struct state_t {
  enum type_t { CONDITIONAL, EXTRACTOR, TERMINATOR };

  type_t type;
  std::string label;

  state_t(type_t _type, std::string _label) : type(_type), label(_label) {}
};

struct conditional_state_t : state_t {
  std::string condition;
  bool transition_value;

  std::shared_ptr<state_t> next_on_true;
  std::shared_ptr<state_t> next_on_false;

  conditional_state_t(const std::string &_label, const std::string &_condition)
      : state_t(CONDITIONAL, _label), condition(_condition) {}
};

struct extractor_state_t : state_t {
  std::string hdr;
  std::string dynamic_length;
  std::shared_ptr<state_t> next;

  extractor_state_t(const std::string &_label, const std::string &_hdr,
                    const std::string &_dynamic_length)
      : state_t(EXTRACTOR, _label), hdr(_hdr), dynamic_length(_dynamic_length) {
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse