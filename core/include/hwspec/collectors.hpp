#pragma once

#include "hwspec/types.hpp"

// Collectors read the machine and return plain data. They never print.
// The Windows implementations live in core/src/windows/. On other systems a stub returns
// empty data, which lets the rest of the code build and be tested anywhere.

namespace hwspec {

CpuInfo collect_cpu();
MemInfo collect_mem();
OsInfo collect_os();

}  // namespace hwspec
