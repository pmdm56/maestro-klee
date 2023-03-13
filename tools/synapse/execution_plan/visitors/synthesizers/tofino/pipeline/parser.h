#pragma once

#include "../../code_builder.h"
#include "../constants.h"
#include "domain/state.h"

#include <assert.h>
#include <iostream>
#include <stack>
#include <vector>

namespace synapse {
namespace synthesizer {
namespace tofino {

class Parser {
private:
  CodeBuilder builder;

  std::shared_ptr<state_t> accept;
  std::shared_ptr<state_t> reject;

  std::stack<std::shared_ptr<state_t>> states_stack;
  std::vector<std::shared_ptr<state_t>> states;

  int state_id;
  bool active;

public:
  Parser(int indentation)
      : builder(indentation),
        accept(new state_t(state_t::type_t::TERMINATOR, "accept")),
        reject(new state_t(state_t::type_t::TERMINATOR, "reject")), state_id(0),
        active(true) {}

  void deactivate() { active = false; }
  bool is_active() const { return active; }

  void transition_accept() {
    assert(active);

    if (states_stack.size() == 0) {
      states.push_back(accept);
      states_stack.push(accept);
    } else {
      transition(accept);
    }
  }

  void transition_reject() {
    assert(active);

    if (states_stack.size() == 0) {
      states.push_back(reject);
      states_stack.push(reject);
    } else {
      transition(reject);
    }
  }

  void push_on_true() {
    assert(active);
    assert(states_stack.size());
    auto current = states_stack.top();

    assert(current->type == state_t::type_t::CONDITIONAL);
    auto conditional = static_cast<conditional_state_t *>(current.get());
    conditional->transition_value = true;
  }

  void push_on_false() {
    assert(active);
    assert(states_stack.size());
    auto current = states_stack.top();

    assert(current->type == state_t::type_t::CONDITIONAL);
    auto conditional = static_cast<conditional_state_t *>(current.get());
    conditional->transition_value = false;
  }

  void pop() {
    assert(active);
    assert(states_stack.size());

    if (states_stack.top()->type == state_t::type_t::CONDITIONAL) {
      auto conditional =
          static_cast<conditional_state_t *>(states_stack.top().get());
      if (conditional->transition_value) {
        return;
      }
    }

    states_stack.pop();
  }

  void add_condition(const std::string &condition) {
    assert(active);

    std::stringstream conditional_label;

    conditional_label << PARSER_CONDITIONAL_LABEL;
    conditional_label << state_id;

    state_id++;

    auto new_stage = std::shared_ptr<state_t>(
        new conditional_state_t(conditional_label.str(), condition));

    transition(new_stage);

    states.push_back(new_stage);
    states_stack.push(new_stage);
  }

  void add_extractor(std::string label) { add_extractor(label, std::string()); }

  void add_extractor(std::string hdr, std::string dynamic_length) {
    assert(active);

    std::stringstream label_stream;
    label_stream << "parse_";
    label_stream << hdr;
    label_stream << "_";
    label_stream << state_id;

    state_id++;

    auto new_stage = std::shared_ptr<state_t>(
        new extractor_state_t(label_stream.str(), hdr, dynamic_length));

    if (states_stack.size()) {
      transition(new_stage);
    }

    states.push_back(new_stage);
    states_stack.push(new_stage);
  }

  void synthesize(std::ostream &os) {
    assert(states.size());
    auto root = states[0];

    builder.indent();
    builder.append("state ");
    builder.append(PARSER_INITIAL_STATE_LABEL);
    builder.append(" {");
    builder.append_new_line();

    builder.inc_indentation();

    builder.indent();
    builder.append("transition ");
    builder.append(root->label);
    builder.append(";");
    builder.append_new_line();

    builder.dec_indentation();

    builder.indent();
    builder.append("}");
    builder.append_new_line();

    auto built_states = std::vector<std::shared_ptr<state_t>>{root};

    while (built_states.size()) {
      auto stage = built_states[0];
      built_states.erase(built_states.begin());

      if (stage->type == state_t::type_t::TERMINATOR) {
        continue;
      }

      builder.append_new_line();

      builder.indent();
      builder.append("state ");
      builder.append(stage->label);
      builder.append(" {");
      builder.append_new_line();

      builder.inc_indentation();

      if (stage->type == state_t::type_t::CONDITIONAL) {
        auto conditional = static_cast<conditional_state_t *>(stage.get());

        builder.indent();
        builder.append("transition select(");
        builder.append(conditional->condition);
        builder.append(") {");
        builder.append_new_line();

        builder.inc_indentation();

        builder.indent();
        assert(conditional->next_on_true || conditional->next_on_false);

        if (conditional->next_on_true) {
          builder.append("true :");
          builder.append(conditional->next_on_true->label);
          builder.append(";");
          builder.append_new_line();

          built_states.push_back(conditional->next_on_false);
        }

        if (conditional->next_on_false) {
          if (conditional->next_on_false->label != "reject") {
            builder.append("false :");
            builder.append(conditional->next_on_false->label);
            builder.append(";");
            builder.append_new_line();
          }

          built_states.push_back(conditional->next_on_true);
        }

        builder.dec_indentation();
        builder.append("}");
        builder.append_new_line();

      } else if (stage->type == state_t::type_t::EXTRACTOR) {
        auto extractor = static_cast<extractor_state_t *>(stage.get());
        assert(extractor->next);

        builder.indent();
        builder.append(PARSER_PACKET_VARIABLE_LABEL);
        builder.append(".extract(hdr.");
        builder.append(extractor->hdr);

        if (extractor->dynamic_length.size()) {
          builder.append(", ");
          builder.append(extractor->dynamic_length);
        }

        builder.append(");");
        builder.append_new_line();

        builder.indent();
        builder.append("transition ");
        builder.append(extractor->next->label);
        builder.append(";");
        builder.append_new_line();

        built_states.push_back(extractor->next);
      }

      builder.dec_indentation();
      builder.indent();
      builder.append("}");
      builder.append_new_line();
    }

    builder.dump(os);
  }

  void print() const {
    if (!states.size()) {
      return;
    }

    std::cerr << "====== states ======\n";
    std::cerr << "current " << states_stack.top()->label << "\n";
    print(states[0]);
    std::cerr << "====================\n\n";
  }

  void print(std::shared_ptr<state_t> stage) const {
    if (!stage) {
      return;
    }

    if (stage->type == state_t::type_t::CONDITIONAL) {
      std::cerr << "stage " << stage->label;
      std::cerr << "\n";

      auto conditional = static_cast<conditional_state_t *>(stage.get());

      std::cerr << "   true:   ";
      if (conditional->next_on_true) {
        std::cerr << conditional->next_on_true->label;
      }
      std::cerr << "\n";

      std::cerr << "   false:  ";
      if (conditional->next_on_false) {
        std::cerr << conditional->next_on_false->label;
      }
      std::cerr << "\n";

      print(conditional->next_on_true);
      print(conditional->next_on_false);
    } else if (stage->type == state_t::type_t::EXTRACTOR) {
      std::cerr << "stage " << stage->label;
      std::cerr << "\n";

      auto extractor = static_cast<extractor_state_t *>(stage.get());

      std::cerr << "   next:   ";
      if (extractor->next) {
        std::cerr << extractor->next->label;
      }
      std::cerr << "\n";

      print(extractor->next);
    }
  }

private:
  void transition(std::shared_ptr<state_t> new_state) {
    assert(active);
    assert(states_stack.size());
    auto current = states_stack.top();

    switch (current->type) {
    case state_t::type_t::CONDITIONAL: {
      auto conditional = static_cast<conditional_state_t *>(current.get());

      if (conditional->transition_value && !conditional->next_on_true) {
        conditional->next_on_true = new_state;
      } else if (!conditional->next_on_false) {
        conditional->next_on_false = new_state;
      }
    } break;

    case state_t::type_t::EXTRACTOR: {
      auto extractor = static_cast<extractor_state_t *>(current.get());

      if (!extractor->next) {
        extractor->next = new_state;
      }
    } break;

    default:
      assert(false && "Should not transition from an already terminated state");
    }
  }
};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse