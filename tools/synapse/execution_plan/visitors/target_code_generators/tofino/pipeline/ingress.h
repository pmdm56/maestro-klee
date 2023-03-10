#pragma once

#include <stack>

#include "../../synthesizer.h"
#include "../constants.h"

#include "domain/stack.h"
#include "domain/table.h"
#include "domain/variable.h"

#include "headers.h"
#include "parser.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

class Ingress {
private:
  std::vector<Variable> intrinsic_metadata;
  std::vector<Variable> user_metadata;
  std::vector<Variable> key_bytes;

  std::stack<bool> pending_ifs;
  std::vector<table_t> tables;

public:
  Synthesizer state_synthesizer;
  Synthesizer apply_block_synthesizer;
  Synthesizer user_metadata_synthesizer;

  Headers headers;
  Parser parser;

  stack_t local_vars;

  Ingress(int state_ind, int apply_block_ind, int user_meta_ind,
          int headers_defs_ind, int headers_decl_ind, int parser_ind)
      : state_synthesizer(state_ind), apply_block_synthesizer(apply_block_ind),
        user_metadata_synthesizer(user_meta_ind),
        headers(headers_defs_ind, headers_decl_ind), parser(parser_ind) {
    intrinsic_metadata = std::vector<Variable>{

        {INGRESS_INTRINSIC_META_RESUBMIT_FLAG,
         INGRESS_INTRINSIC_META_RESUBMIT_FLAG_SIZE_BITS},

        {INGRESS_INTRINSIC_META_PACKET_VERSION,
         INGRESS_INTRINSIC_META_PACKET_VERSION_SIZE_BITS},

        {
            INGRESS_INTRINSIC_META_INGRESS_PORT,
            INGRESS_INTRINSIC_META_INGRESS_PORT_SIZE_BITS,
            {symbex::PORT, symbex::PORT2},
        },

        {INGRESS_INTRINSIC_META_TIMESTAMP,
         INGRESS_INTRINSIC_META_TIMESTAMP_SIZE_BITS},

    };
  }

  void push_pending_if() { pending_ifs.push(true); }
  void pop_pending_if() { pending_ifs.pop(); }

  // returns number of closed stacked blocks
  int close_pending_ifs() {
    int closed = 0;

    if (!pending_ifs.size()) {
      return closed;
    }

    while (pending_ifs.size()) {
      apply_block_synthesizer.dec_indentation();
      apply_block_synthesizer.indent();
      apply_block_synthesizer.append("}");
      apply_block_synthesizer.append_new_line();

      closed++;

      auto if_clause = pending_ifs.top();
      pending_ifs.pop();

      if (if_clause) {
        pending_ifs.push(false);
        break;
      }
    }

    return closed;
  }

  variable_query_t search_variable(std::string symbol) const {
    for (const auto &intrinsic_meta : intrinsic_metadata) {
      if (intrinsic_meta.match(symbol)) {
        return variable_query_t(intrinsic_meta, 0);
      }
    }

    for (const auto &user_meta : user_metadata) {
      if (user_meta.match(symbol)) {
        return variable_query_t(user_meta, 0);
      }
    }

    auto local_var = local_vars.get(symbol);

    if (local_var.valid) {
      return local_var;
    }

    return variable_query_t();
  }

  variable_query_t search_variable(klee::ref<klee::Expr> expr) const {
    for (const auto &intrinsic_meta : intrinsic_metadata) {
      if (intrinsic_meta.match(expr)) {
        return variable_query_t(intrinsic_meta, 0);
      }

      auto contains_result = intrinsic_meta.contains(expr);

      if (contains_result.contains) {
        return variable_query_t(intrinsic_meta, contains_result.offset_bits);
      }
    }

    for (const auto &user_meta : user_metadata) {
      if (user_meta.match(expr)) {
        return variable_query_t(user_meta, 0);
      }

      auto contains_result = user_meta.contains(expr);

      if (contains_result.contains) {
        return variable_query_t(user_meta, contains_result.offset_bits);
      }
    }

    auto local_var = local_vars.get(expr);

    if (local_var.valid) {
      return local_var;
    }

    return variable_query_t();
  }

  Variable allocate_key_byte(int byte) {
    int size = key_bytes.size();

    while (size <= byte) {
      std::stringstream key_byte_label_builder;

      key_byte_label_builder << KEY_BYTE_LABEL;
      key_byte_label_builder << "_";
      key_byte_label_builder << key_bytes.size();

      auto label = key_byte_label_builder.str();
      auto size_bits = 8;

      auto key_byte = Variable(label, size_bits);
      key_byte.synthesize(state_synthesizer);

      key_bytes.push_back(key_byte);
      size = key_bytes.size();
    }

    return key_bytes[byte];
  }

  Variable allocate_meta_param(const std::string &table_label, int param_num,
                               klee::ref<klee::Expr> meta_param) {
    std::stringstream meta_param_label_builder;

    meta_param_label_builder << table_label;
    meta_param_label_builder << "_";
    meta_param_label_builder << param_num;

    auto label = meta_param_label_builder.str();
    auto size_bits = meta_param->getWidth();

    auto meta_param_var = Variable(label, size_bits);
    meta_param_var.set_prefix(INGRESS_USER_METADATA_VARIABLE);
    meta_param_var.add_expr(meta_param);

    meta_param_var.synthesize(user_metadata_synthesizer);

    user_metadata.push_back(meta_param_var);

    return meta_param_var;
  }

  void add_table(const table_t &table) {
    tables.push_back(table);
    table.synthesize(state_synthesizer);
  }

  void synthesize_state(std::ostream &os) const { state_synthesizer.dump(os); }

  void synthesize_apply_block(std::ostream &os) const {
    apply_block_synthesizer.dump(os);
  }

  void synthesize_user_metadata(std::ostream &os) const {
    user_metadata_synthesizer.dump(os);
  }
}; // namespace tofino

} // namespace tofino
} // namespace synthesizer
} // namespace synapse