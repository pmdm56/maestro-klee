#pragma once

#include "../module.h"
#include "else.h"
#include "ignore.h"
#include "then.h"

namespace synapse {
namespace targets {
namespace tofino {

class IfHeaderValid : public Module {
public:
  enum PacketHeader {
    ETHERNET,
    IPV4,
    NOT_IPV4_OPTIONS,
    TCPUDP,
    IGNORE,
  };

private:
  PacketHeader header;

public:
  IfHeaderValid()
      : Module(ModuleType::Tofino_IfHeaderValid, TargetType::Tofino,
               "IfHeaderValid") {}

  IfHeaderValid(BDD::Node_ptr node, PacketHeader _header)
      : Module(ModuleType::Tofino_IfHeaderValid, TargetType::Tofino,
               "IfHeaderValid", node),
        header(_header) {}

private:
  static bool has_only_allowed_symbols(klee::ref<klee::Expr> expr) {
    kutil::RetrieveSymbols retriever;
    retriever.visit(expr);

    auto symbols = retriever.get_retrieved_strings();

    std::vector<std::string> allowed_symbols = {
        synapse::symbex::PACKET_LENGTH,
        synapse::symbex::CHUNK,
    };

    // check if there are unexpected symbols

    for (auto symbol : symbols) {
      auto found_it =
          std::find(allowed_symbols.begin(), allowed_symbols.end(), symbol);

      if (found_it == allowed_symbols.end()) {
        return false;
      }
    }

    return true;
  }

  static bool is_conditional_ipv4(klee::ref<klee::Expr> expr) {
    auto stringified = kutil::expr_to_string(expr, true);
    return stringified == symbex::KLEE_EXPR_IPV4_CONDITION;
  }

  static bool is_conditional_ipv4_options(klee::ref<klee::Expr> expr) {
    auto stringified = kutil::expr_to_string(expr, true);
    return stringified == symbex::KLEE_EXPR_IPV4_OPTIONS_CONDITION;
  }

  static bool
  is_conditional_ipv4_options_to_ignore(klee::ref<klee::Expr> expr) {
    auto stringified = kutil::expr_to_string(expr, true);
    return stringified == symbex::KLEE_EXPR_IPV4_OPTIONS_CONDITION_TO_IGNORE;
  }

  static bool is_conditional_tcpudp(klee::ref<klee::Expr> expr) {
    auto stringified = kutil::expr_to_string(expr, true);
    return stringified == symbex::KLEE_EXPR_TCPUDP_CONDITION ||
           stringified == symbex::KLEE_EXPR_TCPUDP_AFTER_IPV4_OPTIONS_CONDITION;
  }

  processing_result_t process_branch(const ExecutionPlan &ep,
                                     BDD::Node_ptr node,
                                     const BDD::Branch *casted) override {
    processing_result_t result;

    PacketHeader _header;
    auto is_conditional_header =
        is_packet_header_conditional_node(casted, _header);

    if (!is_conditional_header) {
      return result;
    }

    if (_header == IGNORE) {
      auto new_module = std::make_shared<Ignore>(node);

      // Here, we assume next is the correct way to go.
      // FIXME: we should double check this, and actually check
      // if the other path actually leads to a packet drop without any
      // additional operations.
      auto new_ep = ep.ignore_leaf(node->get_next(), TargetType::Tofino);

      result.module = new_module;
      result.next_eps.push_back(new_ep);

      return result;
    }

    auto new_if_module = std::make_shared<IfHeaderValid>(node, _header);

    auto new_then_module = std::make_shared<Then>(node);
    auto new_else_module = std::make_shared<Else>(node);

    auto if_leaf = ExecutionPlan::leaf_t(new_if_module, nullptr);
    auto then_leaf =
        ExecutionPlan::leaf_t(new_then_module, casted->get_on_true());
    auto else_leaf =
        ExecutionPlan::leaf_t(new_else_module, casted->get_on_false());

    std::vector<ExecutionPlan::leaf_t> if_leaves{if_leaf};
    std::vector<ExecutionPlan::leaf_t> then_else_leaves{then_leaf, else_leaf};

    auto ep_if = ep.add_leaves(if_leaves);
    auto ep_if_then_else = ep_if.add_leaves(then_else_leaves);

    result.module = new_if_module;
    result.next_eps.push_back(ep_if_then_else);

    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new IfHeaderValid(node, header);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const IfHeaderValid *>(other);

    if (header != other_cast->get_header()) {
      return false;
    }

    return true;
  }

  PacketHeader get_header() const { return header; }

  static bool is_packet_header_conditional_node(const BDD::Branch *node,
                                                PacketHeader &_header) {
    assert(node);
    auto expr = node->get_condition();

    if (!has_only_allowed_symbols(expr)) {
      return false;
    }

    if (is_conditional_ipv4(expr)) {
      _header = IPV4;
      return true;
    }

    if (is_conditional_ipv4_options(expr)) {
      _header = NOT_IPV4_OPTIONS;
      return true;
    }

    if (is_conditional_ipv4_options_to_ignore(expr)) {
      _header = IGNORE;
      return true;
    }

    if (is_conditional_tcpudp(expr)) {
      _header = TCPUDP;
      return true;
    }

    return false;
  }

  static bool is_packet_header_conditional_node(const BDD::Branch *node) {
    PacketHeader header;
    return is_packet_header_conditional_node(node, header);
  }
};
} // namespace tofino
} // namespace targets
} // namespace synapse
