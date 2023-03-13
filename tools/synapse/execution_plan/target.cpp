#include "target.h"
#include "memory_bank.h"
#include "modules/module.h"

namespace synapse {

Target::Target(TargetType _type, const std::vector<Module_ptr> &_modules,
               const MemoryBank_ptr &_memory_bank)
    : type(_type), modules(_modules), memory_bank(_memory_bank) {}

} // namespace synapse