#pragma once

#include "../../target.h"
#include "../visitor.h"
#include "call-paths-to-bdd.h"

#include <vector>

#define DECLARE_VISIT(M) void visit(const M *node) override;

namespace synapse {

class SearchSpace;

class Graphviz : public ExecutionPlanVisitor {
private:
  std::ofstream ofs;
  std::string fpath;

  const SearchSpace *search_space;
  std::string search_space_fpath;
  std::vector<std::string> bdd_fpaths;

  std::map<TargetType, std::string> node_colors;

  constexpr static int fname_len = 15;
  constexpr static const char *prefix = "/tmp/";
  constexpr static const char *alphanum = "0123456789"
                                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "abcdefghijklmnopqrstuvwxyz";

  std::string get_rand_fname() const;
  void open();

public:
  Graphviz(const std::string &path, const SearchSpace *_search_space);
  Graphviz(const std::string &path);

private:
  Graphviz() : Graphviz(get_rand_fname()) {}
  Graphviz(const SearchSpace *_search_space)
      : Graphviz(get_rand_fname(), _search_space) {
    search_space_fpath = get_rand_fname();
  }

  void function_call(BDD::BDDNode_ptr node, TargetType target,
                     std::string label);

  struct rgb_t {
    int r;
    int g;
    int b;
  };

  rgb_t get_color(float f) const;

  void dump_bdd(const BDD::BDD &bdd,
                const std::unordered_set<uint64_t> &processed,
                const BDD::Node *next);

  std::string get_bdd_node_name(const BDD::Node *node) const;

  void dump_search_space() const;

public:
  static void visualize(const ExecutionPlan &ep, bool interrupt = true);
  static void visualize(const ExecutionPlan &ep, SearchSpace &_search_space,
                        bool interrupt = true);

  ~Graphviz() { ofs.close(); }

  void visit(ExecutionPlan ep) override;
  void visit(const ExecutionPlanNode *ep_node) override;

  void log(const ExecutionPlanNode *ep_node) const override;

  /********************************************
   *
   *                x86 BMv2
   *
   ********************************************/

  DECLARE_VISIT(targets::x86_bmv2::MapGet)
  DECLARE_VISIT(targets::x86_bmv2::CurrentTime)
  DECLARE_VISIT(targets::x86_bmv2::PacketBorrowNextChunk)
  DECLARE_VISIT(targets::x86_bmv2::PacketGetMetadata)
  DECLARE_VISIT(targets::x86_bmv2::PacketReturnChunk)
  DECLARE_VISIT(targets::x86_bmv2::If)
  DECLARE_VISIT(targets::x86_bmv2::Then)
  DECLARE_VISIT(targets::x86_bmv2::Else)
  DECLARE_VISIT(targets::x86_bmv2::Forward)
  DECLARE_VISIT(targets::x86_bmv2::Broadcast)
  DECLARE_VISIT(targets::x86_bmv2::Drop)
  DECLARE_VISIT(targets::x86_bmv2::ExpireItemsSingleMap)
  DECLARE_VISIT(targets::x86_bmv2::RteEtherAddrHash)
  DECLARE_VISIT(targets::x86_bmv2::DchainRejuvenateIndex)
  DECLARE_VISIT(targets::x86_bmv2::VectorBorrow)
  DECLARE_VISIT(targets::x86_bmv2::VectorReturn)
  DECLARE_VISIT(targets::x86_bmv2::DchainAllocateNewIndex)
  DECLARE_VISIT(targets::x86_bmv2::MapPut)
  DECLARE_VISIT(targets::x86_bmv2::PacketGetUnreadLength)
  DECLARE_VISIT(targets::x86_bmv2::SetIpv4UdpTcpChecksum)
  DECLARE_VISIT(targets::x86_bmv2::DchainIsIndexAllocated)

  /********************************************
   *
   *                   BMv2
   *
   ********************************************/

  DECLARE_VISIT(targets::bmv2::SendToController)
  DECLARE_VISIT(targets::bmv2::Ignore)
  DECLARE_VISIT(targets::bmv2::SetupExpirationNotifications)
  DECLARE_VISIT(targets::bmv2::If)
  DECLARE_VISIT(targets::bmv2::Then)
  DECLARE_VISIT(targets::bmv2::Else)
  DECLARE_VISIT(targets::bmv2::EthernetConsume)
  DECLARE_VISIT(targets::bmv2::EthernetModify)
  DECLARE_VISIT(targets::bmv2::TableLookup)
  DECLARE_VISIT(targets::bmv2::IPv4Consume)
  DECLARE_VISIT(targets::bmv2::IPv4Modify)
  DECLARE_VISIT(targets::bmv2::TcpUdpConsume)
  DECLARE_VISIT(targets::bmv2::TcpUdpModify)
  DECLARE_VISIT(targets::bmv2::IPOptionsConsume)
  DECLARE_VISIT(targets::bmv2::IPOptionsModify)
  DECLARE_VISIT(targets::bmv2::Drop)
  DECLARE_VISIT(targets::bmv2::Forward)
  DECLARE_VISIT(targets::bmv2::VectorReturn)

  /********************************************
   *
   *                  Tofino
   *
   ********************************************/

  DECLARE_VISIT(targets::tofino::Ignore)
  DECLARE_VISIT(targets::tofino::If)
  DECLARE_VISIT(targets::tofino::IfHeaderValid)
  DECLARE_VISIT(targets::tofino::Then)
  DECLARE_VISIT(targets::tofino::Else)
  DECLARE_VISIT(targets::tofino::Forward)
  DECLARE_VISIT(targets::tofino::EthernetConsume)
  DECLARE_VISIT(targets::tofino::EthernetModify)
  DECLARE_VISIT(targets::tofino::IPv4Consume)
  DECLARE_VISIT(targets::tofino::IPv4Modify)
  DECLARE_VISIT(targets::tofino::IPv4OptionsConsume)
  DECLARE_VISIT(targets::tofino::IPv4OptionsModify)
  DECLARE_VISIT(targets::tofino::TCPUDPConsume)
  DECLARE_VISIT(targets::tofino::TCPUDPModify)
  DECLARE_VISIT(targets::tofino::IPv4TCPUDPChecksumsUpdate)
  DECLARE_VISIT(targets::tofino::TableLookup)
  DECLARE_VISIT(targets::tofino::TableLookupSimple)
  DECLARE_VISIT(targets::tofino::RegisterRead)
  DECLARE_VISIT(targets::tofino::IntegerAllocatorAllocate)
  DECLARE_VISIT(targets::tofino::IntegerAllocatorRejuvenate)
  DECLARE_VISIT(targets::tofino::IntegerAllocatorQuery)
  DECLARE_VISIT(targets::tofino::Drop)
  DECLARE_VISIT(targets::tofino::SendToController)
  DECLARE_VISIT(targets::tofino::SetupExpirationNotifications)

  /********************************************
   *
   *                x86 Tofino
   *
   ********************************************/

  DECLARE_VISIT(targets::x86_tofino::Ignore)
  DECLARE_VISIT(targets::x86_tofino::PacketParseCPU)
  DECLARE_VISIT(targets::x86_tofino::PacketParseEthernet)
  DECLARE_VISIT(targets::x86_tofino::PacketModifyEthernet)
  DECLARE_VISIT(targets::x86_tofino::ForwardThroughTofino)
  DECLARE_VISIT(targets::x86_tofino::PacketParseIPv4)
  DECLARE_VISIT(targets::x86_tofino::PacketModifyIPv4)
  DECLARE_VISIT(targets::x86_tofino::PacketParseIPv4Options)
  DECLARE_VISIT(targets::x86_tofino::PacketModifyIPv4Options)
  DECLARE_VISIT(targets::x86_tofino::PacketParseTCPUDP)
  DECLARE_VISIT(targets::x86_tofino::PacketModifyTCPUDP)
  DECLARE_VISIT(targets::x86_tofino::PacketModifyChecksums)
  DECLARE_VISIT(targets::x86_tofino::If)
  DECLARE_VISIT(targets::x86_tofino::Then)
  DECLARE_VISIT(targets::x86_tofino::Else)
  DECLARE_VISIT(targets::x86_tofino::Drop)
  DECLARE_VISIT(targets::x86_tofino::MapGet)
  DECLARE_VISIT(targets::x86_tofino::MapPut)
  DECLARE_VISIT(targets::x86_tofino::EtherAddrHash)
  DECLARE_VISIT(targets::x86_tofino::DchainAllocateNewIndex)
  DECLARE_VISIT(targets::x86_tofino::DchainIsIndexAllocated)
  DECLARE_VISIT(targets::x86_tofino::DchainRejuvenateIndex)

private:
  void visit_table(const targets::tofino::TableLookup *node, bool simple);
};
} // namespace synapse
