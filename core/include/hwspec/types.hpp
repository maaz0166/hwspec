#pragma once

#include <cstdint>
#include <optional>
#include <string>

// Plain data returned by the collectors. A value that could not be read stays empty
// (std::optional), and the formatters later print it as "Unknown". Nothing here prints anything.

namespace hwspec {

struct CpuInfo {
  std::optional<std::string> name;
  std::optional<unsigned> physical_cores;
  std::optional<unsigned> logical_cores;
  std::optional<double> base_clock_ghz;
};

struct MemInfo {
  std::optional<std::uint64_t> total_bytes;      // installed memory the OS can use
  std::optional<std::uint64_t> available_bytes;  // memory free right now
};

struct OsInfo {
  std::optional<std::string> name;          // for example "Windows 11 Pro"
  std::optional<std::string> version;       // for example "23H2"
  std::optional<std::string> build;         // for example "22631.3007"
  std::optional<std::string> architecture;  // "x64", "ARM64", "x86"
};

}  // namespace hwspec
