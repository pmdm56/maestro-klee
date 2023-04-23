#pragma once

#include <unordered_set>
#include <vector>

#include "call-paths-to-bdd.h"
#include "klee-util.h"

#include "data_structure.h"

namespace synapse {
namespace targets {
namespace tofino {

class Table;
typedef std::shared_ptr<Table> TableRef;

class Table : public DataStructure {
public:
  struct key_t {
    klee::ref<klee::Expr> expr;
    klee::ref<klee::Expr> condition;
    std::vector<meta_t> meta;

    key_t(klee::ref<klee::Expr> _expr) : expr(_expr) {}
    key_t(klee::ref<klee::Expr> _expr, std::vector<meta_t> _meta)
        : expr(_expr), meta(_meta) {}
  };

  struct param_t {
    std::unordered_set<obj_addr_t> objs;
    std::vector<klee::ref<klee::Expr>> exprs;

    param_t(obj_addr_t obj, klee::ref<klee::Expr> expr) {
      objs.insert(obj);
      exprs.push_back(expr);
    }
  };

protected:
  std::string name;
  std::string base_name;
  std::vector<key_t> keys;
  std::vector<param_t> params;
  std::vector<BDD::symbol_t> hit;

public:
  Table(const std::string &_base_name, const std::vector<key_t> &_keys,
        const std::vector<param_t> &_params,
        const std::vector<BDD::symbol_t> &_hit,
        const std::unordered_set<obj_addr_t> &_objs,
        const std::unordered_set<BDD::node_id_t> &_nodes)
      : DataStructure(Type::TABLE, _objs, _nodes), base_name(_base_name),
        keys(_keys), params(_params), hit(_hit) {
    reset_name();
  }

  Table(const std::vector<key_t> &_keys, const std::vector<param_t> &_params,
        const std::vector<BDD::symbol_t> &_hit,
        const std::unordered_set<obj_addr_t> &_objs,
        const std::unordered_set<BDD::node_id_t> &_nodes)
      : Table("table", _keys, _params, _hit, _objs, _nodes) {}

  virtual void
  add_nodes(const std::unordered_set<BDD::node_id_t> &other_nodes) override {
    DataStructure::add_nodes(other_nodes);
    reset_name();
  }

  void add_hit(const std::vector<BDD::symbol_t> &other_hit) {
    hit.insert(hit.end(), other_hit.begin(), other_hit.end());
  }

  void add_params(const std::vector<Table::param_t> &other_params) {
    // Careful: we actually want to add expression to each param when they match
    // the same objects

    for (const auto &other_param : other_params) {
      const auto &other_objs = other_param.objs;

      auto param_finder = [&](const Table::param_t param) {
        // The stored parameter must contain _all_ the objects referenced by
        // this parameter in order for them to match

        for (const auto &other_obj : other_objs) {
          auto found_it = param.objs.find(other_obj);

          if (found_it == param.objs.end()) {
            return false;
          }
        }

        return true;
      };

      auto param_it = std::find_if(params.begin(), params.end(), param_finder);

      if (param_it != params.end()) {
        param_it->exprs.insert(param_it->exprs.end(), other_param.exprs.begin(),
                               other_param.exprs.end());
      } else {
        params.push_back(other_param);
      }
    }
  }

  bool operator==(const Table &other) const { return name == other.name; }

  const std::string &get_name() const { return name; }
  const std::vector<key_t> &get_keys() const { return keys; }
  const std::vector<param_t> &get_params() const { return params; }
  const std::vector<BDD::symbol_t> &get_hit() const { return hit; }

  static TableRef build(const std::vector<key_t> &_keys,
                        const std::vector<param_t> &_params,
                        const std::vector<BDD::symbol_t> &_hit,
                        const std::unordered_set<obj_addr_t> &_objs,
                        const std::unordered_set<BDD::node_id_t> &_nodes) {
    return TableRef(new Table(_keys, _params, _hit, _objs, _nodes));
  }

  bool equals(const Table *other) const {
    if (!DataStructure::equals(other)) {
      return false;
    }

    if (name != other->get_name()) {
      return false;
    }

    return true;
  }

  virtual bool equals(const DataStructure *other) const override {
    if (!DataStructure::equals(other)) {
      return false;
    }

    auto other_casted = static_cast<const Table *>(other);
    return equals(other_casted);
  }

  virtual bool can_merge(const Table *other) const {
    assert(other);

    auto other_keys = other->get_keys();

    if (keys.size() != other_keys.size()) {
      return false;
    }

    for (auto i = 0u; i < keys.size(); i++) {
      // The keys must have the same metadata

      auto this_meta = keys[i].meta;
      auto other_meta = other_keys[i].meta;

      if (this_meta.size() != other_meta.size()) {
        return false;
      }

      for (auto j = 0u; j < this_meta.size(); j++) {
        if (this_meta[i].symbol != other_meta[i].symbol ||
            this_meta[i].offset != other_meta[i].offset ||
            this_meta[i].size != other_meta[i].size) {
          return false;
        }
      }

      // And access the same packet bytes, in the same order.
      // This check is generally covered by the previous step,
      // but sometimes the same map can be used for multiple checks,
      // and in those cases they will have the same metadata (e.g. bridge
      // with external and internal checks on the map).

      auto this_expr = keys[i].expr;
      auto other_expr = other_keys[i].expr;

      kutil::RetrieveSymbols this_retriever;
      this_retriever.visit(this_expr);
      auto this_chunks = this_retriever.get_retrieved_packet_chunks();

      kutil::RetrieveSymbols other_retriever;
      other_retriever.visit(other_expr);
      auto other_chunks = other_retriever.get_retrieved_packet_chunks();

      if (this_chunks.size() != other_chunks.size()) {
        return false;
      }

      for (auto j = 0u; j < this_chunks.size(); j++) {
        auto this_chunk = this_chunks[j];
        auto other_chunk = other_chunks[j];

        auto same_index = kutil::solver_toolbox.are_exprs_always_equal(
            this_chunk, other_chunk);

        if (!same_index) {
          return false;
        }
      }
    }

    return true;
  }

  virtual void merge(const Table *other) {
    assert(other);

    add_objs(other->get_objs());
    add_nodes(other->get_nodes());
    add_params(other->get_params());
    add_hit(other->get_hit());
  }

  std::ostream &dump(std::ostream &os) const {
    os << "table: " << name << "\n";

    os << "nodes: [";
    for (auto node : nodes) {
      os << node << ",";
    }
    os << "]\n";

    os << "objs: [";
    for (auto obj : objs) {
      os << obj << ",";
    }
    os << "]\n";

    os << "keys:\n";
    for (auto key : keys) {
      os << "  meta: [";
      for (auto meta : key.meta) {
        os << meta.symbol << ",";
      }
      os << "]\n";
      os << "  expr: " << kutil::expr_to_string(key.expr, true) << "\n";
    }

    os << "params:\n";
    for (auto param : params) {
      os << "  objs: ";
      for (auto obj : param.objs) {
        os << obj << ", ";
      }
      os << "\n";
      os << "  exprs: ";
      for (auto expr : param.exprs) {
        os << kutil::expr_to_string(expr, true) << ", ";
      }
      os << "\n";
    }

    os << "hit: [";
    for (auto symbol : hit) {
      os << symbol.label << ",";
    }
    os << "]\n";

    return os;
  }

protected:
  void reset_name() {
    // Use a counter to avoid name collision.
    static int counter = 0;

    std::stringstream table_label_builder;

    table_label_builder << base_name;

    for (auto node_id : nodes) {
      table_label_builder << "_";
      table_label_builder << node_id;
    }

    table_label_builder << "_";
    table_label_builder << counter;

    counter++;

    name = table_label_builder.str();
  }
};

} // namespace tofino
} // namespace targets
} // namespace synapse
