#pragma once

#include "comb_config.h"

namespace nfcomb {

std::ostream &operator<<(std::ostream &os, const combination_config &c) {
  if (c.available) {
    return os << "Configuration: " << c.file_name << std::endl
              << "Graphviz: " << (c.enable_gviz ? "enabled" : "disabled")
              << std::endl
              << "BDD1 color: " << c.bdd1_color << std::endl
              << "BDD2 color: " << c.bdd2_color << std::endl
              << "Prior changes: " << (c.prior_changes ? "BDD2" : "BDD1")
              << std::endl
              << "[Conflict matrix] " << std::endl
              << "              ------- BDD2 --------" << std::endl
              << "      actions | fwd | drop | bcast |" << std::endl
              << "        fwd   |  " << c.conflict_matrix[0][0] << "     "
              << c.conflict_matrix[0][1] << "      " << c.conflict_matrix[0][2]
              << std::endl
              << "BDD1    drop  |  " << c.conflict_matrix[1][0] << "     "
              << c.conflict_matrix[1][1] << "     " << c.conflict_matrix[1][2]
              << std::endl
              << "        bcast |  " << c.conflict_matrix[2][0] << "     "
              << c.conflict_matrix[2][1] << "     " << c.conflict_matrix[2][2]
              << std::endl
              << std::endl;
  } else {
    return os << "[Info] No configuration file was provided. The program will abort if any conflicts are encountered.\n";
  }
}

bool is_hex_color(const std::string &str) {
  if (str.length() != 7 || str[0] != '#') {
    return false;
  }
  for (size_t i = 1; i < str.length(); ++i) {
    if (!std::isxdigit(str[i])) {
      return false;
    }
  }

  return true;
}

void verify_integrity(json j) {

  if (!j.is_discarded()) {

    if (!j.contains("bdd1_color") || !j["bdd1_color"].is_string() ||
        !is_hex_color(j["bdd1_color"]))
      ERROR_MSG("Failed to parse configuration file (bdd1_color).");

    if (!j.contains("bdd2_color") || !j["bdd2_color"].is_string() ||
        !is_hex_color(j["bdd2_color"]))
      ERROR_MSG("Failed to parse configuration file (bdd2_color).");

    if (!j.contains("enable_gviz") || !j["enable_gviz"].is_boolean())
      ERROR_MSG("Failed to parse configuration file (enable_gviz).");

    if (!j.contains("prior_changes") || !j["prior_changes"].is_number())
      ERROR_MSG("Failed to parse configuration file (prior_changes).");
    else if (!(j["prior_changes"] == 0 || j["prior_changes"] == 1))
      ERROR_MSG("Failed to parse configuration file (prior_changes).");

    if (j.contains("conflict_matrix") || j["conflict_matrix"].is_array()) {
      auto matrix = j["conflict_matrix"];
      if (matrix.size() == 3) {
        for (auto line : matrix) {
          if (line.is_array()) {
            if (line.size() == 3) {
              for (auto n : line) {
                if (!n.is_number() || !(n == 0 || n == 1 || n == -1))
                  ERROR_MSG("Failed to parse configuration file (matrix).");
              }
            } else {
              ERROR_MSG("Failed to parse configuration file (matrix).");
            }
          } else {
            ERROR_MSG("Failed to parse configuration file (matrix).");
          }
        }
      } else {
        ERROR_MSG("Failed to parse configuration file (matrix).");
      }
      return;
    }
  }

  ERROR_MSG("Failed to parse configuration file.");
}

combination_config parse_configuration(std::string CONFIG) {

  combination_config ret;

  if (CONFIG.size()) {

    std::ifstream config_file(CONFIG);
    json json_parsed = json::parse(config_file, nullptr, false);

    verify_integrity(json_parsed);

    ret.file_name = CONFIG;
    ret.bdd1_color = json_parsed["bdd1_color"];
    ret.bdd2_color = json_parsed["bdd2_color"];
    ret.enable_gviz = json_parsed["enable_gviz"];
    ret.prior_changes = json_parsed["prior_changes"];

    auto m = json_parsed["conflict_matrix"];
    for (auto line = 0; line < 3; line++)
      for (auto col = 0; col < 3; col++)
        ret.conflict_matrix[line][col] = m[line][col];

    ret.available = true;
  } else {
    ret.available = false;
  }

  return ret;
}

void createGviz(BDD::BDD bdd, combination_config conf, std::string OUT_FILE) {
  auto file = std::ofstream(OUT_FILE + ".gv");
  assert(file.is_open());

  BDD::GraphvizGenerator gv(file);
  gv.set_show_init_graph(true);
  gv.visit(bdd);

  file.close();
}

} // namespace nfcomb