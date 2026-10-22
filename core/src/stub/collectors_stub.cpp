#include "hwspec/collectors.hpp"

// Used only when building on a system that is not Windows (for development and tests).
// hwspec supports Windows only, so these return empty data and every value prints as "Unknown".

namespace hwspec {

CpuInfo collect_cpu() { return {}; }
MemInfo collect_mem() { return {}; }
OsInfo collect_os() { return {}; }

}  // namespace hwspec
