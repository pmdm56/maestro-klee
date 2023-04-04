#include "target.h"
#include "memory_bank.h"
#include "modules/module.h"

namespace synapse {

Target::Target(TargetType _type, const std::vector<Module_ptr> &_modules,
               const MemoryBank_ptr &_memory_bank)
    : type(_type), modules(_modules), memory_bank(_memory_bank) {}

std::ostream &operator<<(std::ostream &os, TargetType type) {
  switch (type) {
  case x86_BMv2: {
    os << "x86_BMv2";
  } break;
  case x86_Tofino: {
    os << "x86_Tofino";
  } break;
  case Tofino: {
    os << "Tofino";
  } break;
  case Netronome: {
    os << "Netronome";
  } break;
  case FPGA: {
    os << "FPGA";
  } break;
  case BMv2: {
    os << "BMv2";
  } break;
  }

  return os;
}

} // namespace synapse