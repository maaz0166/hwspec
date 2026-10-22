#pragma once
#include <string>
#include "hwspec/types.hpp"

namespace hwspec {
std::string json_quote(const std::string& text);
std::string json_cpu(const CpuInfo& info, bool detail);
std::string json_mem(const MemInfo& info, bool detail);
std::string json_os(const OsInfo& info, bool detail);
std::string json_gpu(const GpuInfo& info, bool detail);
std::string json_disk(const DiskInfo& info, bool detail);
std::string json_net(const NetInfo& info, bool detail);
std::string json_battery(const BatteryInfo& info, bool detail);
}  // namespace hwspec
