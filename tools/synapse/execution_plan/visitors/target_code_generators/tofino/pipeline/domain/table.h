#pragma once

#include <string>
#include <vector>

#include "../../../synthesizer.h"
#include "../../constants.h"
#include "variable.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

struct table_t {
  std::string label;
  std::vector<std::string> keys_labels;
  std::vector<Variable> meta_params;

  uint64_t size;

  table_t(std::string _label, std::vector<std::string> _keys_labels,
          std::vector<Variable> _meta_params)
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

  void synthesize(Synthesizer &synthesizer) const {
    synthesizer.append_new_line();

    // =================== Action ===================

    auto action_label = get_action_label();

    synthesizer.indent();

    synthesizer.append("action ");
    synthesizer.append(action_label);
    synthesizer.append("(");

    // ============== Action Arguments ==============

    for (auto i = 0u; i < meta_params.size(); i++) {
      auto param_label = get_param_label(i);
      auto param_type = meta_params[i].get_type();

      if (i != 0) {
        synthesizer.append(", ");
      }

      synthesizer.append(param_type);
      synthesizer.append(" ");
      synthesizer.append(param_label);
    }

    synthesizer.append(") {");
    synthesizer.append_new_line();

    // =============== Action Body ===============

    synthesizer.inc_indentation();

    for (auto i = 0u; i < meta_params.size(); i++) {
      auto meta_label = get_meta_label(i);
      auto param_label = get_param_label(i);

      synthesizer.indent();

      synthesizer.append(meta_label);
      synthesizer.append(" = ");
      synthesizer.append(param_label);
      synthesizer.append(";");
      synthesizer.append_new_line();
    }

    synthesizer.dec_indentation();
    synthesizer.indent();

    synthesizer.append("}");
    synthesizer.append_new_line();

    // ================== Table ==================

    synthesizer.append_new_line();

    synthesizer.indent();

    synthesizer.append("table ");
    synthesizer.append(label);
    synthesizer.append(" {");
    synthesizer.append_new_line();

    synthesizer.inc_indentation();

    // ================ Table Keys ================

    synthesizer.indent();

    synthesizer.append("key = {");
    synthesizer.append_new_line();

    synthesizer.inc_indentation();

    for (auto key_label : keys_labels) {
      synthesizer.indent();

      synthesizer.append(key_label);
      synthesizer.append(": exact;");
      synthesizer.append_new_line();
    }

    synthesizer.dec_indentation();

    synthesizer.indent();

    synthesizer.append("}");
    synthesizer.append_new_line();

    // ================ Table Actions ================

    synthesizer.append_new_line();

    synthesizer.indent();

    synthesizer.append("actions = {");
    synthesizer.append_new_line();

    synthesizer.inc_indentation();
    synthesizer.indent();

    synthesizer.append(action_label);
    synthesizer.append(";");
    synthesizer.append_new_line();

    synthesizer.dec_indentation();
    synthesizer.indent();

    synthesizer.append("}");
    synthesizer.append_new_line();

    // =============== Table Properties ===============

    synthesizer.append_new_line();
    synthesizer.indent();

    synthesizer.append("size = ");
    synthesizer.append(size);
    synthesizer.append(";");
    synthesizer.append_new_line();

    synthesizer.indent();

    synthesizer.append("idle_timout = true;");
    synthesizer.append_new_line();

    // ================= End Table =================

    synthesizer.dec_indentation();

    synthesizer.indent();

    synthesizer.append("}");
    synthesizer.append_new_line();
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse