#pragma once

#include "../../../symbex.h"
#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_tofino {

typedef uint16_t cpu_code_path_t;

class PacketParseCPU : public Module {
private:
  std::vector<BDD::symbols_t> dataplane_state;

public:
  PacketParseCPU()
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU") {}

  PacketParseCPU(BDD::Node_ptr node,
                 const std::vector<BDD::symbols_t> &_dataplane_state)
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU", node),
        dataplane_state(_dataplane_state) {}

  PacketParseCPU(
      BDD::Node_ptr node,
      const std::vector<std::vector<BDD::symbol_t>> &_dataplane_states)
      : Module(ModuleType::x86_Tofino_PacketParseCPU, TargetType::x86_Tofino,
               "PacketParseCPU", node) {
    unify_dataplane_states(_dataplane_states);
  }

private:
  void unify_dataplane_states(
      const std::vector<std::vector<BDD::symbol_t>> &dataplane_states) {
    std::unordered_map<std::string, std::vector<BDD::symbol_t>> unified_by_base;

    for (auto ds : dataplane_states) {
      for (auto s : ds) {
        unified_by_base[s.label_base].push_back(s);
      }
    }

    for (auto it = unified_by_base.begin(); it != unified_by_base.end(); it++) {
      dataplane_state.emplace_back();

      for (auto s : it->second) {
        if (dataplane_state.back().size()) {
          auto present = dataplane_state.back().begin();
          assert(s.label_base == present->label_base);
          assert(!s.expr.isNull());
          assert(s.expr->getWidth() == present->expr->getWidth());
        }

        dataplane_state.back().insert(s);
      }
    }
  }

  void dump_dataplane_state() const {
    std::cerr << "Unified:\n";
    for (auto ss : dataplane_state) {
      std::cerr << "================================\n";
      for (auto s : ss) {
        std::cerr << s.label << " " << s.label_base;
        if (!s.expr.isNull()) {
          std::cerr << " " << kutil::expr_to_string(s.expr, true);
        }
        std::cerr << "\n";
      }
    }
  }

  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;
    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const override {
    visitor.visit(ep_node, this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new PacketParseCPU(node, dataplane_state);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    return true;
  }

  const std::vector<BDD::symbols_t> &get_dataplane_state() const {
    return dataplane_state;
  }
};
} // namespace x86_tofino
} // namespace targets
} // namespace synapse
