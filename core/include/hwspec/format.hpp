#pragma once
#include <cstdint>
#include <string>
#include "hwspec/report.hpp"
#include "hwspec/types.hpp"

namespace hwspec {
std::string format_bytes(std::uint64_t bytes);
std::string format_ghz(double ghz);
SectionReport format_cpu(const CpuInfo& info, bool detail = false);
SectionReport format_mem(const MemInfo& info, bool detail = false);
SectionReport format_os(const OsInfo& info, bool detail = false);
SectionReport format_gpu(const GpuInfo& info, bool detail = false);
SectionReport format_disk(const DiskInfo& info, bool detail = false);
SectionReport format_net(const NetInfo& info, bool detail = false);
SectionReport format_battery(const BatteryInfo& info, bool detail = false);
}  // namespace hwspec
