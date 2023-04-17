#pragma once

#include <string>
#include <vector>

#include "../../../code_builder.h"
#include "../../constants.h"
#include "../domain/variable.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

struct port_allocator_t {
  std::string label;
  std::vector<std::string> keys_labels;
  Variables meta_params;

  uint64_t size;

  port_allocator_t(std::string _label, std::vector<std::string> _keys_labels,
                   Variables _meta_params)
      : label(_label), keys_labels(_keys_labels), meta_params(_meta_params),
        size(1024) {}

  std::string get_action_label() const {
    std::stringstream action_label;
    action_label << label << "_populate";
    return action_label.str();
  }

  std::string get_param_label(int i) const {
    std::stringstream param_label;
    param_label << "param_" << i;
    return param_label.str();
  }

  std::string get_meta_label(int i) const {
    assert(i < (int)meta_params.size());
    return meta_params[i].get_label();
  }

  void synthesize(CodeBuilder &builder) const { assert(false && "TODO"); }

  bool operator==(const port_allocator_t &other) const {
    return label == other.label;
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse