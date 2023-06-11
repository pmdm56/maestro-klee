#pragma once

#include "emulator/emulator.h"

#include "call-paths-to-bdd.h"
#include <unordered_map>

namespace BDD {

inline std::unordered_map<node_id_t, emulation::hit_rate_t>
get_hit_rate(const BDD &bdd, const emulation::cfg_t &cfg,
             const std::string &pcap_file, uint16_t device) {
  emulation::Emulator emulator(bdd, cfg);
  emulator.run(pcap_file, device);
  return emulator.get_hit_rate();
}

} // namespace BDD
