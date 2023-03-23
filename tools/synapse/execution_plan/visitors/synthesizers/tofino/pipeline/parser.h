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
  std::stack<bool> active;

public:
  Parser(int indentation)
      : builder(indentation), accept(new terminator_state_t("accept")),
        reject(new terminator_state_t("reject")), state_id(0), active() {
    active.push(true);

    auto starter = std::shared_ptr<state_t>(new initial_state_t());
    states.push_back(starter);
    states_stack.push(starter);
  }

  void deactivate() {
    assert(active.size());
    active.top() = false;
  }

  bool is_active() const {
    assert(active.size());
    return active.top();
  }

  void transition_accept() {
    assert(is_active());
    transition(accept);
  }

  void transition_reject() {
    assert(is_active());
    transition(reject);
  }

  void push_on_true() {
    assert(is_active());
    assert(states_stack.size());
    auto current = states_stack.top();

    assert(current->type == state_t::type_t::CONDITIONAL);
    auto conditional = static_cast<conditional_state_t *>(current.get());
    conditional->transition_value = true;
  }

  void push_on_false() {
    assert(is_active());
    assert(states_stack.size());
    auto current = states_stack.top();

    assert(current->type == state_t::type_t::CONDITIONAL);
    auto conditional = static_cast<conditional_state_t *>(current.get());
    conditional->transition_value = false;
  }

  void pop() {
    assert(active.size());
    assert(states_stack.size());

    if (states_stack.top()->type == state_t::type_t::CONDITIONAL) {
      auto conditional =
          static_cast<conditional_state_t *>(states_stack.top().get());
      if (conditional->transition_value) {
        return;
      }
    }

    states_stack.pop();
    active.pop();
  }

  void add_condition(const std::string &condition) {
    assert(is_active());

    std::stringstream conditional_label;

    conditional_label << PARSER_CONDITIONAL_LABEL;
    conditional_label << state_id;

    state_id++;

    auto new_stage = std::shared_ptr<state_t>(
        new conditional_state_t(conditional_label.str(), condition));

    transition(new_stage);

    states.push_back(new_stage);
    states_stack.push(new_stage);
    active.push(true);
  }

  void add_extractor(std::string label) { add_extractor(label, std::string()); }

  void add_extractor(std::string hdr, std::string dynamic_length) {
    assert(is_active());

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
    active.push(true);
  }

  void synthesize(std::ostream &os) {
    assert(states.size());
    auto root = states[0];

    auto built_stages = std::vector<std::shared_ptr<state_t>>{root};

    while (built_stages.size()) {
      auto stage = built_stages[0];
      built_stages.erase(built_stages.begin());

      stage->synthesize(builder);

      switch (stage->type) {
      case state_t::type_t::INITIAL: {
        auto initial = static_cast<initial_state_t *>(stage.get());
        assert(initial->next);
        built_stages.push_back(initial->next);
      } break;

      case state_t::type_t::CONDITIONAL: {
        auto conditional = static_cast<conditional_state_t *>(stage.get());
        assert(conditional->next_on_true || conditional->next_on_false);

        if (conditional->next_on_true) {
          built_stages.push_back(conditional->next_on_false);
        }

        if (conditional->next_on_false) {
          built_stages.push_back(conditional->next_on_true);
        }

      } break;

      case state_t::type_t::EXTRACTOR: {
        auto extractor = static_cast<extractor_state_t *>(stage.get());
        assert(extractor->next);
        built_stages.push_back(extractor->next);
      } break;

      case state_t::type_t::TERMINATOR: {
        // do nothing
      } break;
      }
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
    assert(is_active());
    assert(states_stack.size());
    auto current = states_stack.top();

    switch (current->type) {
    case state_t::type_t::INITIAL: {
      auto initial = static_cast<initial_state_t *>(current.get());
      assert(!initial->next);
      initial->next = new_state;
    } break;

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