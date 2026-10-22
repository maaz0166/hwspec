#pragma once
#include "hwspec/types.hpp"

namespace hwspec {
CpuInfo collect_cpu(bool detail = false);
MemInfo collect_mem(bool detail = false);
OsInfo collect_os(bool detail = false);
GpuInfo collect_gpu(bool detail = false);
DiskInfo collect_disk(bool detail = false);
NetInfo collect_net(bool detail = false);
BatteryInfo collect_battery(bool detail = false);
}  // namespace hwspec
