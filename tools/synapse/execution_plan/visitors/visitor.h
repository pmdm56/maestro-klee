#pragma once

#include <memory>

namespace synapse {

class ExecutionPlan;
class ExecutionPlanNode;

namespace targets {
namespace x86_bmv2 {
class MapGet;
class CurrentTime;
class PacketBorrowNextChunk;
class PacketGetMetadata;
class PacketReturnChunk;
class If;
class Then;
class Else;
class Forward;
class Broadcast;
class Drop;
class ExpireItemsSingleMap;
class RteEtherAddrHash;
class DchainRejuvenateIndex;
class VectorBorrow;
class VectorReturn;
class DchainAllocateNewIndex;
class MapPut;
class PacketGetUnreadLength;
class SetIpv4UdpTcpChecksum;
class DchainIsIndexAllocated;
} // namespace x86_bmv2

namespace bmv2 {
class SendToController;
class ParserConsume;
class Ignore;
class SetupExpirationNotifications;
class If;
class Then;
class Else;
class EthernetConsume;
class EthernetModify;
class TableLookup;
class IPv4Consume;
class IPv4Modify;
class TcpUdpConsume;
class TcpUdpModify;
class IPOptionsConsume;
class IPOptionsModify;
class Drop;
class Forward;
class VectorReturn;
} // namespace bmv2

namespace tofino {
class Drop;
class Forward;
class EthernetConsume;
class IPv4Consume;
class TcpUdpConsume;
class Ignore;
class SetupExpirationNotifications;
class If;
class Then;
class Else;
class EthernetModify;
class IPv4Modify;
class TcpUdpModify;
class IPOptionsConsume;
class IPOptionsModify;
class TableLookup;
class UpdateIpv4TcpUdpChecksum;
class PortAllocatorAllocate;
class CachedTableLookup;
} // namespace tofino
} // namespace targets

class ExecutionPlanVisitor {
public:
  virtual void visit(ExecutionPlan ep);
  virtual void visit(const ExecutionPlanNode *ep_node);

  /*************************************
   *
   *              x86
   *
   * **********************************/

  virtual void visit(const targets::x86_bmv2::MapGet *node) {}
  virtual void visit(const targets::x86_bmv2::CurrentTime *node) {}
  virtual void visit(const targets::x86_bmv2::PacketBorrowNextChunk *node) {}
  virtual void visit(const targets::x86_bmv2::PacketReturnChunk *node) {}
  virtual void visit(const targets::x86_bmv2::PacketGetMetadata *node) {}
  virtual void visit(const targets::x86_bmv2::If *node) {}
  virtual void visit(const targets::x86_bmv2::Then *node) {}
  virtual void visit(const targets::x86_bmv2::Else *node) {}
  virtual void visit(const targets::x86_bmv2::Forward *node) {}
  virtual void visit(const targets::x86_bmv2::Broadcast *node) {}
  virtual void visit(const targets::x86_bmv2::Drop *node) {}
  virtual void visit(const targets::x86_bmv2::ExpireItemsSingleMap *node) {}
  virtual void visit(const targets::x86_bmv2::RteEtherAddrHash *node) {}
  virtual void visit(const targets::x86_bmv2::DchainRejuvenateIndex *node) {}
  virtual void visit(const targets::x86_bmv2::VectorBorrow *node) {}
  virtual void visit(const targets::x86_bmv2::VectorReturn *node) {}
  virtual void visit(const targets::x86_bmv2::DchainAllocateNewIndex *node) {}
  virtual void visit(const targets::x86_bmv2::MapPut *node) {}
  virtual void visit(const targets::x86_bmv2::PacketGetUnreadLength *node) {}
  virtual void visit(const targets::x86_bmv2::SetIpv4UdpTcpChecksum *node) {}
  virtual void visit(const targets::x86_bmv2::DchainIsIndexAllocated *node) {}

  /*************************************
   *
   *        BMv2SimpleSwitchgRPC
   *
   * **********************************/

  virtual void
  visit(const targets::bmv2::SendToController *node) {}
  virtual void visit(const targets::bmv2::Ignore *node) {}
  virtual void visit(
      const targets::bmv2::SetupExpirationNotifications *node) {
  }
  virtual void visit(const targets::bmv2::If *node) {}
  virtual void visit(const targets::bmv2::Then *node) {}
  virtual void visit(const targets::bmv2::Else *node) {}
  virtual void
  visit(const targets::bmv2::EthernetConsume *node) {}
  virtual void
  visit(const targets::bmv2::EthernetModify *node) {}
  virtual void visit(const targets::bmv2::TableLookup *node) {}
  virtual void visit(const targets::bmv2::IPv4Consume *node) {}
  virtual void visit(const targets::bmv2::IPv4Modify *node) {}
  virtual void visit(const targets::bmv2::TcpUdpConsume *node) {
  }
  virtual void visit(const targets::bmv2::TcpUdpModify *node) {}
  virtual void
  visit(const targets::bmv2::IPOptionsConsume *node) {}
  virtual void
  visit(const targets::bmv2::IPOptionsModify *node) {}
  virtual void visit(const targets::bmv2::Drop *node) {}
  virtual void visit(const targets::bmv2::Forward *node) {}
  virtual void visit(const targets::bmv2::VectorReturn *node) {}

  /*************************************
   *
   *              Tofino
   *
   * **********************************/

  virtual void visit(const targets::tofino::Drop *node) {}
  virtual void visit(const targets::tofino::Forward *node) {}
  virtual void visit(const targets::tofino::EthernetConsume *node) {}
  virtual void visit(const targets::tofino::IPv4Consume *node) {}
  virtual void visit(const targets::tofino::TcpUdpConsume *node) {}
  virtual void visit(const targets::tofino::Ignore *node) {}
  virtual void
  visit(const targets::tofino::SetupExpirationNotifications *node) {}
  virtual void visit(const targets::tofino::If *node) {}
  virtual void visit(const targets::tofino::Then *node) {}
  virtual void visit(const targets::tofino::Else *node) {}
  virtual void visit(const targets::tofino::EthernetModify *node) {}
  virtual void visit(const targets::tofino::IPv4Modify *node) {}
  virtual void visit(const targets::tofino::TcpUdpModify *node) {}
  virtual void visit(const targets::tofino::IPOptionsConsume *node) {}
  virtual void visit(const targets::tofino::IPOptionsModify *node) {}
  virtual void visit(const targets::tofino::TableLookup *node) {}
  virtual void visit(const targets::tofino::UpdateIpv4TcpUdpChecksum *node) {}
  virtual void visit(const targets::tofino::PortAllocatorAllocate *node) {}
  virtual void visit(const targets::tofino::CachedTableLookup *node) {}
};

} // namespace synapse
