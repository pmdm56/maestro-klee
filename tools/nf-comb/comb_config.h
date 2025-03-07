#pragma once

#include "json.hpp"
#include <fstream>
#include "error_utils.h"

using namespace nlohmann;

namespace nfcomb {

struct combination_config {
  bool available;
  int conflict_matrix[3][3];
  int prior_changes;
  bool enable_gviz;
  std::string bdd1_color;
  std::string bdd2_color;
  std::string file_name;
};

std::ostream &operator<<(std::ostream &os, const combination_config &c);

combination_config parse_configuration(std::string CONFIG);

} // namespace nfcomb