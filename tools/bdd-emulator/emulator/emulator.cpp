#include "emulator.h"

#include "pcap.h"

namespace BDD {
namespace emulation {

void Emulator::list_operations() const {
  std::cerr << "Known operations:\n";
  for (auto it = operations.begin(); it != operations.end(); it++) {
    std::cerr << "  " << it->first << "\n";
  }
}

void Emulator::dump_context(const context_t &ctx) const {
  std::cerr << "========================================\n";
  std::cerr << "Context:\n";
  for (auto c : ctx) {
    std::cerr << "  [*] " << kutil::expr_to_string(c, true) << "\n";
  }
  std::cerr << "========================================\n";
}

void remember_device(uint16_t device, context_t &ctx) {
  auto width = symbex::PORT_SYMBOL_SIZE;
  auto device_symbol =
      kutil::solver_toolbox.create_new_symbol(symbex::PORT, width);
  concretize(ctx, device_symbol, device);
}

void remember_pkt_len(uint32_t len, context_t &ctx) {
  auto width = symbex::PACKET_LENGTH_SYMBOL_SIZE;
  auto length_symbol =
      kutil::solver_toolbox.create_new_symbol(symbex::PACKET_LENGTH, width);
  concretize(ctx, length_symbol, len);
}

bool Emulator::evaluate_condition(klee::ref<klee::Expr> condition,
                                  context_t &ctx) const {
  auto always_true = kutil::solver_toolbox.is_expr_always_true(ctx, condition);

  // FIXME: remove this
  // {
  //   auto always_false =
  //       kutil::solver_toolbox.is_expr_always_false(ctx, condition);
  //   auto maybe_true = kutil::solver_toolbox.is_expr_maybe_true(ctx, condition);
  //   auto maybe_false = kutil::solver_toolbox.is_expr_maybe_true(ctx, condition);
  //   std::cerr << "condition: " << kutil::expr_to_string(condition, true)
  //             << "\n";
  //   std::cerr << "always true?  " << always_true << "\n";
  //   std::cerr << "always false? " << always_false << "\n";
  //   std::cerr << "maybe true?   " << maybe_true << "\n";
  //   std::cerr << "maybe false?  " << maybe_false << "\n";
  // }

  assert(((kutil::solver_toolbox.is_expr_always_true(ctx, condition)) ^
          (kutil::solver_toolbox.is_expr_always_false(ctx, condition))) &&
         "Can't be sure...");

  return always_true;
}

void Emulator::run(pkt_t pkt, time_ns_t time, uint16_t device) {
  context_t ctx;
  auto next_node = bdd.get_process();

  remember_device(device, ctx);
  remember_pkt_len(pkt.size, ctx);

  while (next_node) {
    auto node = next_node;
    next_node = nullptr;

    auto id = node->get_id();
    meta.hit_counter[id]++;

    // {
    //   std::cerr << "Node: " << id << "\n";
    //   dump_context(ctx);
    // }

    switch (node->get_type()) {
    case Node::CALL: {
      auto call_node = cast_node<Call>(node);
      assert(call_node);

      auto call = call_node->get_call();
      auto operation = get_operation(call.function_name);

      operation(call_node, pkt, time, state, ctx, cfg);

      next_node = node->get_next();
    } break;
    case Node::BRANCH: {
      auto branch_node = cast_node<Branch>(node);
      assert(branch_node);

      auto condition = branch_node->get_condition();
      auto result = evaluate_condition(condition, ctx);

      if (result) {
        next_node = branch_node->get_on_true();
      } else {
        next_node = branch_node->get_on_false();
      }
    } break;
    case Node::RETURN_PROCESS: {
      auto ret_node = cast_node<ReturnProcess>(node);
      assert(ret_node);

      auto op = ret_node->get_return_operation();

      if (op == ReturnProcess::Operation::DROP) {
        meta.rejected++;
      } else {
        meta.accepted++;
      }
    } break;
    default: {
      assert(false && "Should not be here.");
      std::cerr << "Error: run in debug mode.\n";
      exit(1);
    }
    }
  }
}

void Emulator::run(const std::string &pcap_file, uint16_t device) {
  char errbuff[PCAP_ERRBUF_SIZE];
  pcap_t *pcap = pcap_open_offline(pcap_file.c_str(), errbuff);

  if (pcap == nullptr) {
    std::cerr << "Error opening pcap file " << pcap_file << ": " << errbuff
              << "\n";
    exit(1);
  }

  pcap_pkthdr *header;
  const u_char *data;
  int status = 1;
  time_ns_t time = 0;

  while ((status = pcap_next_ex(pcap, &header, &data)) >= 0) {
    pkt_t pkt;

    pkt.data = static_cast<const uint8_t *>(data);
    pkt.size = static_cast<uint32_t>(header->len);

    if (cfg.rate.set) {
      // To obtain the time in seconds:
      // (pkt.size * 8) / (gbps * 1e9)
      // We want in ns, not seconds, so we multiply by 1e9.
      // This cancels with the 1e9 on the bottom side of the operation.
      // So actually, result in ns = (pkt.size * 8) / gbps
      time += (pkt.size * 8) / cfg.rate.value;
    } else {
      time = header->ts.tv_sec * 1e9 + header->ts.tv_usec * 1e6;
    }

    // std::cerr << "------------- NEW PACKET! -------------\n";
    run(pkt, time, device);

    meta.packet_counter++;
    std::cerr << "Processed packets: " << meta.packet_counter << "\r";
    // {
    //   char c;
    //   std::cin >> c;
    // }
  }
  std::cerr << "\n";
}

std::unordered_map<node_id_t, hit_rate_t> Emulator::get_hit_rate() const {
  std::unordered_map<node_id_t, hit_rate_t> hit_rate;

  for (auto it = meta.hit_counter.begin(); it != meta.hit_counter.end(); it++) {
    if (meta.packet_counter > 0) {
      hit_rate[it->first] = it->second / (float)(meta.packet_counter);
    } else {
      hit_rate[it->first] = 0;
    }
  }

  return hit_rate;
}

operation_ptr Emulator::get_operation(const std::string &name) const {
  auto operation_it = operations.find(name);

  if (operation_it == operations.end()) {
    std::cerr << "Unknown operation \"" << name << "\"\n";
    assert(false && "Unknown operation");
    exit(1);
  }

  return *operation_it->second;
}

void Emulator::setup() {
  auto node = bdd.get_init();

  while (node) {
    if (node->get_type() == Node::CALL) {
      auto call_node = cast_node<Call>(node);
      assert(call_node);

      auto call = call_node->get_call();
      auto operation = get_operation(call.function_name);

      pkt_t mock_pkt;
      context_t empty_ctx;

      operation(call_node, mock_pkt, 0, state, empty_ctx, cfg);

      node = node->get_next();
    } else if (node->get_type() == Node::BRANCH) {
      auto branch_node = cast_node<Branch>(node);
      assert(branch_node);
      node = branch_node->get_on_true();
    } else {
      break;
    }
  }
}

} // namespace emulation
} // namespace BDD