#pragma once

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "call-paths-to-bdd.h"

namespace BDD {

struct callpath_hitrate_report_t {
  std::unordered_map<uint64_t, uint64_t> entries;

  callpath_hitrate_report_t(const std::string &filename) {
    std::string line;
    auto stream = std::ifstream(filename);
    assert(stream.is_open());

    while (!stream.eof()) {
      std::getline(stream, line);

      if (line.size() == 0 || line[0] == '#') {
        continue;
      }

      auto delim = line.find('\t');
      auto id_str = line.substr(0, delim);
      auto hits_str = line.substr(delim + 1);

      auto id = std::stoull(id_str);
      auto hits = std::stoll(hits_str);

      entries[id] = hits;
    }

    normalize();
  }

  void normalize() {
    uint64_t total_hits = 0;

    for (auto kv = entries.begin(); kv != entries.end(); kv++) {
      total_hits += kv->second;
      // printf("id %2u hits %lu\n", entry.call_path_id, entry.hits);
    }

    // printf("\n");

    for (auto kv = entries.begin(); kv != entries.end(); kv++) {
      kv->second = (uint64_t)((double)100 * kv->second / total_hits);
      // printf("id %2u hits %3lu %%\n", entry.call_path_id, entry.hits);
    }
  }
};

class CallPathHitRateGraphvizGenerator : public BDDVisitor {
private:
  std::ostream &os;
  callpath_hitrate_report_t report;

public:
  CallPathHitRateGraphvizGenerator(std::ostream &_os,
                                   const callpath_hitrate_report_t &_report)
      : os(_os), report(_report) {}

private:
  struct color_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    color_t(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b) {}

    std::string to_gv_repr() const {
      std::stringstream ss;

      ss << "\"";
      ss << "#";
      ss << std::hex;

      ss << std::setw(2);
      ss << std::setfill('0');
      ss << (int)r;

      ss << std::setw(2);
      ss << std::setfill('0');
      ss << (int)g;

      ss << std::setw(2);
      ss << std::setfill('0');
      ss << (int)b;

      ss << "\"";

      return ss.str();
    }
  };

  std::string get_node_color(const Node *node) {
    uint64_t cumulative_hit_rate = 0;

    // Use call paths filenames to check their ID.
    // Don't forget that their ID starts with 1 instead of 0 though...
    auto term_ids = node->get_terminating_node_ids();
    for (auto id : term_ids) {
      assert(report.entries.find(id) != report.entries.end());
      cumulative_hit_rate += report.entries[id];
    }

    float hit_rate_fp = (float)cumulative_hit_rate / 100.0;

    auto blue = color_t(0, 0, 1);
    auto cyan = color_t(0, 1, 1);
    auto green = color_t(0, 1, 0);
    auto yellow = color_t(1, 1, 0);
    auto red = color_t(1, 0, 0);

    auto palette = std::vector<color_t>{blue, cyan, green, yellow, red};

    auto value = hit_rate_fp * (palette.size() - 1);
    auto idx1 = (int)std::floor(value);
    auto idx2 = (int)idx1 + 1;
    auto frac = value - idx1;

    auto r =
        0xff * ((palette[idx2].r - palette[idx1].r) * frac + palette[idx1].r);
    auto g =
        0xff * ((palette[idx2].g - palette[idx1].g) * frac + palette[idx1].g);
    auto b =
        0xff * ((palette[idx2].b - palette[idx1].b) * frac + palette[idx1].b);

    auto color = color_t(r, g, b);
    return color.to_gv_repr();
  }

  std::string get_gv_name(const Node *node) const {
    assert(node);

    std::stringstream stream;

    if (node->get_type() == Node::NodeType::RETURN_INIT) {
      const ReturnInit *ret = static_cast<const ReturnInit *>(node);

      stream << "\"return ";
      switch (ret->get_return_value()) {
      case ReturnInit::ReturnType::SUCCESS: {
        stream << "1";
        break;
      }
      case ReturnInit::ReturnType::FAILURE: {
        stream << "0";
        break;
      }
      default: {
        assert(false);
      }
      }
      stream << "\"";

      return stream.str();
    }

    stream << node->get_id();
    return stream.str();
  }

public:
  Action visitBranch(const Branch *node) override {
    if (node->get_next()) {
      assert(node->get_on_true()->get_prev());
      assert(node->get_on_true()->get_prev()->get_id() == node->get_id());

      assert(node->get_on_false()->get_prev());
      assert(node->get_on_false()->get_prev()->get_id() == node->get_id());
    }

    auto condition = node->get_condition();

    assert(node->get_on_true());
    node->get_on_true()->visit(*this);

    assert(node->get_on_false());
    node->get_on_false()->visit(*this);

    os << "\t\t" << get_gv_name(node);
    os << " [shape=Mdiamond, label=\"";

    os << node->get_id() << ":";
    os << kutil::pretty_print_expr(condition);

    os << "\"";
    os << ", color=" << get_node_color(node);

    os << "];\n";

    os << "\t\t" << get_gv_name(node);
    os << " -> ";
    os << get_gv_name(node->get_on_true().get());
    os << " [label=\"True\"];\n";

    os << "\t\t" << get_gv_name(node);
    os << " -> ";
    os << get_gv_name(node->get_on_false().get());
    os << " [label=\"False\"];\n";

    return STOP;
  }

  Action visitCall(const Call *node) override {
    if (node->get_next()) {
      if (!node->get_next()->get_prev()) {
        std::cerr << "ERROR IN " << node->dump(true) << "\n";
        std::cerr << " => " << node->get_next()->dump(true) << "\n";
      }
      assert(node->get_next()->get_prev());
      assert(node->get_next()->get_prev()->get_id() == node->get_id());
    }
    auto call = node->get_call();

    assert(node->get_next());
    node->get_next()->visit(*this);

    os << "\t\t" << get_gv_name(node);
    os << " [label=\"";
    auto i = 0u;

    os << node->get_id() << ":";
    os << call.function_name;
    os << "(";

    i = 0;
    for (const auto &pair : call.args) {
      if (call.args.size() > 1) {
        os << "\\l";
        os << std::string(2, ' ');
      }

      os << pair.first << ":";
      arg_t arg = pair.second;

      if (arg.fn_ptr_name.first) {
        os << arg.fn_ptr_name.second;
      } else {
        os << kutil::pretty_print_expr(arg.expr);

        if (!arg.in.isNull() || !arg.out.isNull()) {
          os << "[";

          if (!arg.in.isNull()) {
            os << kutil::pretty_print_expr(arg.in);
          }

          if (!arg.out.isNull() &&
              (arg.in.isNull() || !kutil::solver_toolbox.are_exprs_always_equal(
                                      arg.in, arg.out))) {
            os << " -> ";
            os << kutil::pretty_print_expr(arg.out);
          }

          os << "]";
        }
      }

      if (i != call.args.size() - 1) {
        os << ",";
      }

      // os << kutil::pretty_print_expr(arg.expr);

      i++;
    }

    os << ")\\l";

    os << "\", ";
    os << "color=" << get_node_color(node);

    os << "];\n";

    os << "\t\t" << get_gv_name(node);
    os << " -> ";
    os << get_gv_name(node->get_next().get());
    os << ";\n";

    return STOP;
  }

  Action visitReturnInit(const ReturnInit *node) override {
    auto value = node->get_return_value();

    os << "\t\t\"return ";
    switch (value) {
    case ReturnInit::ReturnType::SUCCESS: {
      os << "1\"";

      os << " [color=" << get_node_color(node) << "]";

      break;
    }
    case ReturnInit::ReturnType::FAILURE: {
      os << "0\"";
      os << " [color=" << get_node_color(node) << "]";

      break;
    }
    default: {
      assert(false);
    }
    }
    os << ";\n";

    return STOP;
  }

  Action visitReturnProcess(const ReturnProcess *node) override {
    auto value = node->get_return_value();
    auto operation = node->get_return_operation();

    os << "\t\t" << get_gv_name(node);
    os << " [label=\"";
    os << node->get_id() << ":";
    switch (operation) {
    case ReturnProcess::Operation::FWD: {
      os << "fwd(" << value << ")";
      os << "\", ";
      os << "color=" << get_node_color(node) << "]";

      break;
    }
    case ReturnProcess::Operation::DROP: {
      os << "drop()";
      os << "\", ";
      os << "color=" << get_node_color(node) << "]";

      break;
    }
    case ReturnProcess::Operation::BCAST: {
      os << "bcast()";
      os << "\", ";
      os << "color=" << get_node_color(node) << "]";

      break;
    }
    default: {
      assert(false);
    }
    }
    os << ";\n";
    return STOP;
  }

  void visitInitRoot(const Node *root) override {
    os << "digraph mygraph {\n";
    os << "\tnode [shape=box];\n";

    os << "\tsubgraph clusterinit {\n";
    os << "\t\tlabel=\"nf_init\";\n";
    os << "\t\tnode [style=filled,color=" << get_node_color(root) << "];\n";

    root->visit(*this);
  }

  void visitProcessRoot(const Node *root) override {
    os << "\t}\n";

    os << "\tsubgraph clusterprocess {\n";
    os << "\t\tlabel=\"nf_process\"\n";
    os << "\t\tnode [style=filled,color=" << get_node_color(root) << "];\n";

    root->visit(*this);

    os << "\t}\n";
    os << "}";
  }
};
} // namespace BDD
