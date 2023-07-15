#pragma once

#include "json.hpp"
#include "bdd/visitors/graphviz-generator.h"
#include <fstream>

namespace nfcomb {

struct combination_config {
  int conflict_matrix[3][3];
  int prior_changes;
  bool enable_gviz;
  std::string bdd1_color;
  std::string bdd2_color;
  std::string file_name;
};

std::ostream &operator<<(std::ostream &os, const combination_config &c) {
  return os << "Configuration: " << c.file_name << std::endl
            << "Graphviz: " << (c.enable_gviz ? "enabled" : "disabled")
            << std::endl
            << "BDD1 color: " << c.bdd1_color << std::endl
            << "BDD2 color: " << c.bdd2_color << std::endl
            << "Prior changes: " << (c.prior_changes ? "BDD2" : "BDD1")
            << std::endl
            << "Conflict matrix: " << std::endl
            << c.conflict_matrix[0][0] << "  " << c.conflict_matrix[0][1]
            << "  " << c.conflict_matrix[0][2] << std::endl
            << c.conflict_matrix[1][0] << "  " << c.conflict_matrix[1][1]
            << "  " << c.conflict_matrix[1][2] << std::endl
            << c.conflict_matrix[2][0] << "  " << c.conflict_matrix[2][1]
            << "  " << c.conflict_matrix[2][2] << std::endl << std::endl;
}

combination_config parse_configuration(std::string CONFIG) {
  nlohmann::json j;
  std::ifstream config_file(CONFIG);
  auto json_parsed = j.parse(config_file);

  combination_config ret;

  ret.file_name = CONFIG;
  ret.bdd1_color = *json_parsed.find("bdd1_color");
  ret.bdd2_color = *json_parsed.find("bdd2_color");
  ret.enable_gviz = *json_parsed.find("enable_gviz");
  ret.prior_changes = *json_parsed.find("prior_changes");

  auto m = *json_parsed.find("conflict_matrix");
  for (auto line = 0; line < 3; line++)
    for (auto col = 0; col < 3; col++)
      ret.conflict_matrix[line][col] = m[line][col];

  return ret;
}

void createGviz(BDD::BDD bdd, combination_config conf, std::string OUT_FILE){
  auto file = std::ofstream(OUT_FILE + ".gv");
  assert(file.is_open());
  
  BDD::GraphvizGenerator gv(file);
  gv.set_show_init_graph(true);
  gv.visit(bdd);

  file.close();
 
}

} // namespace nfcomb