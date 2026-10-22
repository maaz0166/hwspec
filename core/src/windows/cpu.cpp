#include <string>
#include <vector>

#include "hwspec/collectors.hpp"
#include "registry.hpp"

namespace hwspec {

namespace {

// Windows stores one key per logical processor; processor 0 is always there.
const char* const kCpuKey = R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)";

std::string trim(const std::string& text) {
  const char* const whitespace = " \t\r\n";
  const std::size_t first = text.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = text.find_last_not_of(whitespace);
  return text.substr(first, last - first + 1);
}

// Physical cores = how many "processor core" records Windows reports.
std::optional<unsigned> count_physical_cores() {
  DWORD length = 0;
  // The first call fails on purpose and tells us how big the buffer must be.
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || length == 0) {
    return std::nullopt;
  }

  std::vector<char> buffer(length);
  if (!GetLogicalProcessorInformationEx(
          RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length)) {
    return std::nullopt;
  }

  // The buffer holds several records of different sizes, one after another.
  unsigned count = 0;
  DWORD offset = 0;
  while (offset < length) {
    const auto* record = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);
    if (record->Size == 0) {
      break;
    }
    if (record->Relationship == RelationProcessorCore) {
      ++count;
    }
    offset += record->Size;
  }
  if (count == 0) {
    return std::nullopt;
  }
  return count;
}

}  // namespace

CpuInfo collect_cpu() {
  CpuInfo info;

  if (const auto name = win::reg_string(kCpuKey, "ProcessorNameString")) {
    const std::string cleaned = trim(*name);  // some CPUs pad the name with spaces
    if (!cleaned.empty()) {
      info.name = cleaned;
    }
  }

  // "~MHz" is the CPU's rated speed in MHz, which we show as the base clock.
  if (const auto mhz = win::reg_dword(kCpuKey, "~MHz")) {
    if (*mhz > 0) {
      info.base_clock_ghz = static_cast<double>(*mhz) / 1000.0;
    }
  }

  info.physical_cores = count_physical_cores();

  // Logical processors (threads), counted across all processor groups.
  const DWORD logical = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if (logical > 0) {
    info.logical_cores = static_cast<unsigned>(logical);
  }

  return info;
}

}  // namespace hwspec
