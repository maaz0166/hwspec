#include "hwspec/format.hpp"

#include <algorithm>
#include <cstdio>

namespace hwspec {

namespace {

const char* const kUnknown = "Unknown";

std::string or_unknown(const std::optional<std::string>& value) {
  return value ? *value : std::string(kUnknown);
}

}  // namespace

std::string format_bytes(std::uint64_t bytes) {
  static const char* const units[] = {"B", "KB", "MB", "GB", "TB"};
  if (bytes < 1024) {
    return std::to_string(bytes) + " B";
  }
  double value = static_cast<double>(bytes);
  int unit = 0;
  while (value >= 1024.0 && unit < 4) {
    value /= 1024.0;
    ++unit;
  }
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.1f %s", value, units[unit]);
  return buffer;
}

std::string format_ghz(double ghz) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.2f GHz", ghz);
  return buffer;
}

SectionReport format_cpu(const CpuInfo& info) {
  SectionReport report;
  report.title = "CPU";

  report.fields.push_back({"Name", or_unknown(info.name)});

  std::string cores = kUnknown;
  if (info.physical_cores && info.logical_cores) {
    cores = std::to_string(*info.physical_cores) + " physical, " + std::to_string(*info.logical_cores) + " logical";
  } else if (info.physical_cores) {
    cores = std::to_string(*info.physical_cores) + " physical";
  } else if (info.logical_cores) {
    cores = std::to_string(*info.logical_cores) + " logical";
  }
  report.fields.push_back({"Cores", cores});

  report.fields.push_back({"Base clock", info.base_clock_ghz ? format_ghz(*info.base_clock_ghz) : kUnknown});
  return report;
}

SectionReport format_mem(const MemInfo& info) {
  SectionReport report;
  report.title = "Memory";

  if (!info.total_bytes) {
    report.fields = {{"Total", kUnknown}, {"Used", kUnknown}, {"Available", kUnknown}};
    return report;
  }

  const std::uint64_t total = *info.total_bytes;
  report.fields.push_back({"Total", format_bytes(total)});

  if (!info.available_bytes) {
    report.fields.push_back({"Used", kUnknown});
    report.fields.push_back({"Available", kUnknown});
    return report;
  }

  const std::uint64_t available = std::min(*info.available_bytes, total);
  const std::uint64_t used = total - available;
  const std::uint64_t percent = total > 0 ? (used * 100 + total / 2) / total : 0;  // rounded to the nearest percent

  report.fields.push_back({"Used", format_bytes(used) + " (" + std::to_string(percent) + "%)"});
  report.fields.push_back({"Available", format_bytes(available)});
  return report;
}

SectionReport format_os(const OsInfo& info) {
  SectionReport report;
  report.title = "OS";
  report.fields.push_back({"Name", or_unknown(info.name)});
  report.fields.push_back({"Version", or_unknown(info.version)});
  report.fields.push_back({"Build", or_unknown(info.build)});
  report.fields.push_back({"Architecture", or_unknown(info.architecture)});
  return report;
}

}  // namespace hwspec
