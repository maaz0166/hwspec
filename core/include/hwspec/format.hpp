#pragma once

#include <cstdint>
#include <string>

#include "hwspec/report.hpp"
#include "hwspec/types.hpp"

namespace hwspec {

// "15.9 GB". Windows (Task Manager, Settings) divides by 1024 and still writes GB,
// so we follow that convention to show the same numbers people already know.
std::string format_bytes(std::uint64_t bytes);

// "3.40 GHz"
std::string format_ghz(double ghz);

// Each formatter turns collected data into a report. Missing values become "Unknown".
SectionReport format_cpu(const CpuInfo& info);
SectionReport format_mem(const MemInfo& info);
SectionReport format_os(const OsInfo& info);

}  // namespace hwspec
