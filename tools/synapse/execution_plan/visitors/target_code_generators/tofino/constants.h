#pragma once

#include "pipeline/domain/header.h"

namespace synapse {
namespace synthesizer {
namespace tofino {

constexpr char TOFINO_BOILERPLATE_FILE[] = "boilerplate.p4";

constexpr char MARKER_HEADERS_DEFINITIONS[] = "HEADERS DEFINITIONS";
constexpr char MARKER_INGRESS_METADATA[] = "INGRESS METADATA";
constexpr char MARKER_INGRESS_HEADERS[] = "INGRESS HEADERS";
constexpr char MARKER_EGRESS_METADATA[] = "EGRESS METADATA";
constexpr char MARKER_EGRESS_HEADERS[] = "EGRESS HEADERS";
constexpr char MARKER_INGRESS_PARSE_HEADERS[] = "INGRESS PARSE HEADERS";
constexpr char MARKER_INGRESS_STATE[] = "INGRESS STATE";
constexpr char MARKER_INGRESS_APPLY[] = "INGRESS APPLY";

constexpr char INGRESS_FORWARD_ACTION[] = "fwd";
constexpr char INGRESS_DROP_ACTION[] = "drop";
constexpr char INGRESS_FLOOD_ACTION[] = "flood";
constexpr char INGRESS_SEND_TO_CPU_ACTION[] = "send_to_cpu";

constexpr char PARSER_CONDITIONAL_LABEL[] = "conditional_state_t_";
constexpr char PARSER_PACKET_VARIABLE_LABEL[] = "pkt";
constexpr char PARSER_INITIAL_STATE_LABEL[] = "parse_headers";

constexpr char INGRESS_INTRINSIC_METADATA_VARIABLE[] = "ig_intr_md";
constexpr char INGRESS_USER_METADATA_VARIABLE[] = "meta";

constexpr char HDR_ETH[] = "ethernet";
constexpr char HDR_IPV4[] = "ipv4";
constexpr char HDR_TCPUDP[] = "tcpudp";

constexpr char HDR_ETH_SRC_ADDR_FIELD[] = "src_addr";
constexpr char HDR_ETH_DST_ADDR_FIELD[] = "dst_addr";
constexpr char HDR_ETH_ETHER_TYPE_FIELD[] = "ether_type";

constexpr char HDR_IPV4_VERSION_FIELD[] = "version";
constexpr char HDR_IPV4_IHL_FIELD[] = "ihl";
constexpr char HDR_IPV4_DSCP_FIELD[] = "dscp";
constexpr char HDR_IPV4_TOTAL_LEN_FIELD[] = "total_len";
constexpr char HDR_IPV4_ID_FIELD[] = "identification";
constexpr char HDR_IPV4_FLAGS_FIELD[] = "flags";
constexpr char HDR_IPV4_FRAG_OFF_FIELD[] = "frag_offset";
constexpr char HDR_IPV4_TTL_FIELD[] = "ttl";
constexpr char HDR_IPV4_PROTO_FIELD[] = "protocol";
constexpr char HDR_IPV4_CHKSUM_FIELD[] = "hdr_checksum";
constexpr char HDR_IPV4_SRC_ADDR_FIELD[] = "src_addr";
constexpr char HDR_IPV4_DST_ADDR_FIELD[] = "dst_addr";

constexpr char HDR_TCPUDP_SRC_PORT_FIELD[] = "src_port";
constexpr char HDR_TCPUDP_DST_PORT_FIELD[] = "dst_port";

const header_field_t eth_dst_addr{HDR_ETH_SRC_ADDR_FIELD, 48};
const header_field_t eth_src_addr{HDR_ETH_DST_ADDR_FIELD, 48};
const header_field_t eth_ether_type{HDR_ETH_ETHER_TYPE_FIELD, 16};

} // namespace tofino
} // namespace synthesizer
} // namespace synapse