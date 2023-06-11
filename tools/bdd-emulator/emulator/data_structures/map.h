#pragma once

#include "byte.h"
#include "data_structure.h"

#include <unordered_map>

namespace BDD {
namespace emulation {

class Map : public DataStructure {
private:
  std::unordered_map<bytes_t, int, bytes_t::hash> data;
  uint64_t capacity;

public:
  Map(addr_t _obj, uint64_t _capacity)
      : DataStructure(MAP, _obj), capacity(_capacity) {}

  bool get(bytes_t key, int &value) const {
    if (contains(key)) {
      value = data.at(key);
      return true;
    }

    return false;
  }

  bool contains(bytes_t key) const { return data.find(key) != data.end(); }

  void put(bytes_t key, int value) {
    assert(data.size() <= capacity);
    data[key] = value;
  }

  void erase(bytes_t key) {
    auto removed = data.erase(key);
    assert(removed > 0);
  }

  static Map *cast(const DataStructureRef &ds) {
    assert(ds->get_type() == DataStructureType::MAP);
    return static_cast<Map *>(ds.get());
  }
};

} // namespace emulation
} // namespace BDD