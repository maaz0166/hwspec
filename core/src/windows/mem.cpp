#include "hwspec/collectors.hpp"
#include "registry.hpp"  // brings in windows.h with the right settings

namespace hwspec {

MemInfo collect_mem() {
  MemInfo info;

  MEMORYSTATUSEX status = {};
  status.dwLength = sizeof(status);  // Windows requires the size to be set before the call
  if (GlobalMemoryStatusEx(&status)) {
    info.total_bytes = static_cast<std::uint64_t>(status.ullTotalPhys);
    info.available_bytes = static_cast<std::uint64_t>(status.ullAvailPhys);
  }
  return info;
}

}  // namespace hwspec
