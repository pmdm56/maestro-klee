#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

typedef bit<9>  port_t;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> ether_type_t;
typedef bit<8>  ip_protocol_t;

const port_t RECIRCULATING_PORT = 68;

// BFN-T10-032D
// BFN-T10-032D-024
// BFN-T10-032D-020
// BFN-T10-032D-018
// const PortId_t CPU_PCIE_PORT = 192;

// BFN-T10-064Q
// BFN-T10-032Q
// PCIe port when using the tofino model
const PortId_t CPU_PCIE_PORT = 320;

const ether_type_t  ETHERTYPE_IPV4 = 0x0800;
const ether_type_t  ETHERTYPE_ARP  = 0x0806;
const ether_type_t  ETHERTYPE_IPV6 = 0x86dd;
const ether_type_t  ETHERTYPE_VLAN = 0x8100;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP  = 6;
const ip_protocol_t IP_PROTOCOLS_UDP  = 17;

{{HEADERS DEFINITIONS}}

struct my_ingress_metadata_t {
  {{INGRESS METADATA}}
}

struct my_ingress_headers_t {
  {{INGRESS HEADERS}}
}

struct my_egress_metadata_t {
  {{EGRESS METADATA}}
}

struct my_egress_headers_t {
  {{EGRESS HEADERS}}
}

parser TofinoIngressParser(
    packet_in pkt,
    out ingress_intrinsic_metadata_t ig_intr_md)
{
  state start {
    pkt.extract(ig_intr_md);
    transition select(ig_intr_md.resubmit_flag) {
      1 : parse_resubmit;
      0 : parse_port_metadata;
    }
  }

  state parse_resubmit {
    // Parse resubmitted packet here.
    transition reject;
  }

  state parse_port_metadata {
    pkt.advance(PORT_METADATA_SIZE);
    transition accept;
  }
}

parser IngressParser(
  packet_in pkt,
  out my_ingress_headers_t  hdr,
  out my_ingress_metadata_t meta,
  out ingress_intrinsic_metadata_t ig_intr_md)
{
  TofinoIngressParser() tofino_parser;
  
  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    tofino_parser.apply(pkt, ig_intr_md);

    transition select(ig_intr_md.ingress_port) {
      default: parse_headers;
    }
  }

  {{INGRESS PARSE HEADERS}} 
}

control Ingress(
  inout my_ingress_headers_t  hdr,
  inout my_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_t ig_intr_md,
  in    ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
  inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
  inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
  {{INGRESS STATE}}

  action flood() {
    ig_tm_md.mcast_grp_a = 1;
  }

  action fwd(port_t port){
    ig_tm_md.ucast_egress_port = port;
  }

  action drop() {
    ig_dprsr_md.drop_ctl = 1;
  }

  action send_to_cpu(){
    fwd(CPU_PCIE_PORT);
  }

  apply {
    {{INGRESS APPLY}}
  }
}

control IngressDeparser(
  packet_out pkt,
  inout my_ingress_headers_t hdr,
  in    my_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
{
  apply {
    pkt.emit(hdr);
  }
}

parser TofinoEgressParser(
    packet_in pkt,
    out egress_intrinsic_metadata_t eg_intr_md)
{
  state start {
    pkt.extract(eg_intr_md);
    transition accept;
  }
}

parser EgressParser(
    packet_in pkt,
    out my_egress_headers_t hdr,
    out my_egress_metadata_t eg_md,
    out egress_intrinsic_metadata_t eg_intr_md)
{
  TofinoEgressParser() tofino_parser;

  /* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, eg_intr_md);
    transition accept;
	}
}

control Egress(
    inout my_egress_headers_t hdr,
    inout my_egress_metadata_t eg_md,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
    inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md)
{
  apply {}
}

control EgressDeparser(
    packet_out pkt,
    inout my_egress_headers_t hdr,
    in my_egress_metadata_t eg_md,
    in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md)
{
  apply {
    pkt.emit(hdr);
  }
}

Pipeline(
  IngressParser(),
  Ingress(),
  IngressDeparser(),
  EgressParser(),
  Egress(),
  EgressDeparser()
) pipe;

Switch(pipe) main;
