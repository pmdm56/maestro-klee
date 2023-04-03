#pragma once

namespace synapse {
namespace symbex {

constexpr char CHUNK[] = "packet_chunks";
constexpr char PORT[] = "VIGOR_DEVICE";
constexpr char PORT2[] = "device";
constexpr char CPU_CODE_PATH[] = "cpu_code_path";
constexpr char PACKET_LENGTH[] = "pkt_len";

constexpr char FN_CURRENT_TIME[] = "current_time";
constexpr char FN_ETHER_HASH[] = "rte_ether_addr_hash";
constexpr char FN_ETHER_HASH_ARG_OBJ[] = "obj";
constexpr char FN_EXPIRE_MAP[] = "expire_items_single_map";
constexpr char EXPIRE_MAP_FREED_FLOWS[] = "number_of_freed_flows";

constexpr char FN_SET_CHECKSUM[] = "nf_set_rte_ipv4_udptcp_checksum";
constexpr char FN_SET_CHECKSUM_ARG_IP[] = "ip_header";
constexpr char FN_SET_CHECKSUM_ARG_L4[] = "l4_header";
constexpr char FN_SET_CHECKSUM_ARG_PACKET[] = "packet";
constexpr char CHECKSUM[] = "checksum";

constexpr char FN_BORROW_CHUNK[] = "packet_borrow_next_chunk";
constexpr char FN_RETURN_CHUNK[] = "packet_return_chunk";
constexpr char FN_GET_UNREAD_LEN[] = "packet_get_unread_length";

constexpr char FN_BORROW_ARG_PACKET[] = "p";
constexpr char FN_BORROW_ARG_CHUNK[] = "chunk";

constexpr char FN_DCHAIN_ALLOCATE[] = "dchain_allocate";
constexpr char FN_DCHAIN_ALLOCATE_ARG_CHAIN_OUT[] = "chain_out";
constexpr char FN_DCHAIN_ALLOCATE_ARG_INDEX_RANGE[] = "index_range";
constexpr char FN_DCHAIN_REJUVENATE[] = "dchain_rejuvenate_index";
constexpr char FN_DCHAIN_ALLOCATE_NEW_INDEX[] = "dchain_allocate_new_index";
constexpr char FN_DCHAIN_IS_ALLOCATED[] = "dchain_is_index_allocated";
constexpr char FN_DCHAIN_ARG_CHAIN[] = "chain";
constexpr char FN_DCHAIN_ARG_TIME[] = "time";
constexpr char FN_DCHAIN_ARG_INDEX[] = "index";
constexpr char FN_DCHAIN_ARG_OUT[] = "index_out";
constexpr char DCHAIN_OUT_OF_SPACE[] = "out_of_space";
constexpr char DCHAIN_NEW_INDEX[] = "new_index";
constexpr char DCHAIN_IS_INDEX_ALLOCATED[] = "dchain_is_index_allocated";

constexpr char FN_MAP_GET[] = "map_get";
constexpr char FN_MAP_PUT[] = "map_put";
constexpr char FN_MAP_ARG_MAP[] = "map";
constexpr char FN_MAP_ARG_KEY[] = "key";
constexpr char FN_MAP_ARG_VALUE[] = "value";
constexpr char FN_MAP_ARG_OUT[] = "value_out";
constexpr char MAP_HAS_THIS_KEY[] = "map_has_this_key";
constexpr char MAP_ALLOCATED_INDEX[] = "allocated_index";

constexpr char FN_VECTOR_BORROW[] = "vector_borrow";
constexpr char FN_VECTOR_RETURN[] = "vector_return";
constexpr char FN_VECTOR_ARG_VECTOR[] = "vector";
constexpr char FN_VECTOR_ARG_INDEX[] = "index";
constexpr char FN_VECTOR_ARG_VALUE[] = "value";
constexpr char FN_VECTOR_ARG_OUT[] = "val_out";
constexpr char FN_VECTOR_EXTRA[] = "borrowed_cell";
constexpr char VECTOR_VALUE_SYMBOL[] = "vector_data_reset";

constexpr char FN_BORROW_CHUNK_ARG_LEN[] = "length";
constexpr char FN_BORROW_CHUNK_EXTRA[] = "the_chunk";

constexpr char FN_EXPIRE_MAP_ARG_CHAIN[] = "chain";
constexpr char FN_EXPIRE_MAP_ARG_VECTOR[] = "vector";
constexpr char FN_EXPIRE_MAP_ARG_MAP[] = "map";
constexpr char FN_EXPIRE_MAP_ARG_TIME[] = "time";

} // namespace symbex
} // namespace synapse