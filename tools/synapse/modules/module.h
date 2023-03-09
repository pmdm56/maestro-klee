#pragma once

#include "call-paths-to-bdd.h"

#include "../execution_plan/visitors/visitor.h"
#include "../log.h"
#include "../symbex.h"

#define MODULE(X) (std::make_shared<X>())
#define UINT_16_SWAP_ENDIANNESS(p) ((((p)&0xff) << 8) | ((p) >> 8 & 0xff))

namespace synapse {

enum Target {
  x86_BMv2,
  x86_Tofino,
  Tofino,
  Netronome,
  FPGA,
  BMv2,
};

class ExecutionPlan;
class Module;

typedef std::shared_ptr<Module> Module_ptr;

struct processing_result_t {
  std::vector<ExecutionPlan> next_eps;
  Module_ptr module;
};

class Module {
public:
  enum ModuleType {
    x86_BMv2_CurrentTime,
    x86_BMv2_IfThen,
    x86_BMv2_If,
    x86_BMv2_Then,
    x86_BMv2_Else,
    x86_BMv2_MapGet,
    x86_BMv2_PacketBorrowNextChunk,
    x86_BMv2_PacketGetMetadata,
    x86_BMv2_PacketReturnChunk,
    x86_BMv2_Forward,
    x86_BMv2_Drop,
    x86_BMv2_Broadcast,
    x86_BMv2_ExpireItemsSingleMap,
    x86_BMv2_RteEtherAddrHash,
    x86_BMv2_DchainRejuvenateIndex,
    x86_BMv2_VectorBorrow,
    x86_BMv2_VectorReturn,
    x86_BMv2_DchainAllocateNewIndex,
    x86_BMv2_MapPut,
    x86_BMv2_PacketGetUnreadLength,
    x86_BMv2_SetIpv4UdpTcpChecksum,
    x86_BMv2_DchainIsIndexAllocated,
    BMv2_SendToController,
    BMv2_Ignore,
    BMv2_SetupExpirationNotifications,
    BMv2_If,
    BMv2_Then,
    BMv2_Else,
    BMv2_EthernetConsume,
    BMv2_EthernetModify,
    BMv2_TableLookup,
    BMv2_TableMatch,
    BMv2_TableMiss,
    BMv2_IPv4Consume,
    BMv2_IPv4Modify,
    BMv2_TcpUdpConsume,
    BMv2_TcpUdpModify,
    BMv2_IPOptionsConsume,
    BMv2_IPOptionsModify,
    BMv2_Drop,
    BMv2_Forward,
    BMv2_VectorReturn,
    Tofino_Ignore,
    Tofino_If,
    Tofino_Then,
    Tofino_Else,
    Tofino_Forward,
    Tofino_EthernetConsume,
    Tofino_EthernetModify,
    Tofino_TableLookup,
    Tofino_Drop,
    Tofino_SendToController,
  };

protected:
  struct modification_t {
    unsigned byte;
    klee::ref<klee::Expr> expr;

    modification_t(unsigned _byte, klee::ref<klee::Expr> _expr)
        : byte(_byte), expr(_expr) {}

    modification_t(const modification_t &modification)
        : byte(modification.byte), expr(modification.expr) {}
  };

protected:
  ModuleType type;
  Target target;
  Target next_target;
  const char *name;
  BDD::BDDNode_ptr node;

protected:
  Module(ModuleType _type, Target _target, const char *_name,
         BDD::BDDNode_ptr _node)
      : type(_type), target(_target), next_target(_target), name(_name),
        node(_node) {}

  Module(ModuleType _type, Target _target, const char *_name)
      : type(_type), target(_target), next_target(_target), name(_name),
        node(nullptr) {}

public:
  Module() {}
  Module(const Module &m) : Module(m.type, m.target, m.name, m.node) {}

  ModuleType get_type() const { return type; }

  const char *get_name() const { return name; }

  Target get_target() const { return target; }
  Target get_next_target() const { return next_target; }

  BDD::BDDNode_ptr get_node() const {
    if (!node) {
      std::cerr << "\n";
      std::cerr << get_target_name() << "::" << get_name() << "NO NODE\n";
    }
    assert(node);
    return node;
  }

  void replace_node(BDD::BDDNode_ptr _node) {
    node = _node;
    assert(node);
  }

  static std::string target_to_string(Target target) {
    switch (target) {
    case x86_BMv2:
      return "x86 (BMv2)";
    case x86_Tofino:
      return "x86 (Tofino)";
    case Tofino:
      return "Tofino";
    case Netronome:
      return "Netronome";
    case FPGA:
      return "FPGA";
    case BMv2:
      return "BMv2";
    }

    assert(false && "I should not be here");
    exit(1);
  }

  std::string get_target_name() const { return target_to_string(target); }

  processing_result_t process_node(const ExecutionPlan &_ep,
                                   BDD::BDDNode_ptr node, int max_reordered);

  virtual void visit(ExecutionPlanVisitor &visitor) const = 0;
  virtual Module_ptr clone() const = 0;
  virtual bool equals(const Module *other) const = 0;

protected:
  // Shared module functionality
  virtual processing_result_t process_branch(const ExecutionPlan &ep,
                                             BDD::BDDNode_ptr node,
                                             const BDD::Branch *casted);

  virtual processing_result_t process_call(const ExecutionPlan &ep,
                                           BDD::BDDNode_ptr node,
                                           const BDD::Call *casted);

  virtual processing_result_t
  process_return_init(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
                      const BDD::ReturnInit *casted);

  virtual processing_result_t
  process_return_process(const ExecutionPlan &ep, BDD::BDDNode_ptr node,
                         const BDD::ReturnProcess *casted);

protected:
  // General useful queries
  bool query_contains_map_has_key(const BDD::Branch *node) const;
  BDD::BDDNode_ptr
  get_past_node_that_generates_symbol(const BDD::Node *current_node,
                                      const std::string &symbol) const;
  std::vector<BDD::BDDNode_ptr>
  get_all_prev_functions(const BDD::Node *node,
                         const std::string &function_name);
  std::vector<modification_t>
  build_modifications(klee::ref<klee::Expr> before,
                      klee::ref<klee::Expr> after) const;
};

} // namespace synapse
