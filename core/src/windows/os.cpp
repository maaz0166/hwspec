#include <cstdlib>
#include <string>

#include "hwspec/collectors.hpp"
#include "registry.hpp"

namespace hwspec {

namespace {

const char* const kVersionKey = R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)";

// Windows 11 still reports "Windows 10 ..." in the registry (a known quirk).
// Builds from 22000 upwards are Windows 11, so we fix the name ourselves.
std::string fix_windows_11_name(const std::string& name, unsigned long build) {
  const std::string old_prefix = "Windows 10";
  if (build >= 22000 && name.compare(0, old_prefix.size(), old_prefix) == 0) {
    return "Windows 11" + name.substr(old_prefix.size());
  }
  return name;
}

std::optional<std::string> architecture_name() {
  SYSTEM_INFO system_info = {};
  GetNativeSystemInfo(&system_info);  // the real CPU architecture, even for 32-bit programs
  switch (system_info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      return std::string("x64");
    case PROCESSOR_ARCHITECTURE_ARM64:
      return std::string("ARM64");
    case PROCESSOR_ARCHITECTURE_INTEL:
      return std::string("x86");
    case PROCESSOR_ARCHITECTURE_ARM:
      return std::string("ARM");
    default:
      return std::nullopt;
  }
}

}  // namespace

OsInfo collect_os() {
  OsInfo info;

  const auto build_number = win::reg_string(kVersionKey, "CurrentBuildNumber");
  const unsigned long build_value = build_number ? std::strtoul(build_number->c_str(), nullptr, 10) : 0;

  if (const auto name = win::reg_string(kVersionKey, "ProductName")) {
    info.name = fix_windows_11_name(*name, build_value);
  }

  // "23H2" style version; older Windows 10 releases only have "ReleaseId".
  if (const auto display = win::reg_string(kVersionKey, "DisplayVersion")) {
    info.version = *display;
  } else if (const auto release = win::reg_string(kVersionKey, "ReleaseId")) {
    info.version = *release;
  }

  if (build_number) {
    std::string build = *build_number;
    if (const auto revision = win::reg_dword(kVersionKey, "UBR")) {
      build += "." + std::to_string(*revision);  // update build revision, as winver shows it
    }
    info.build = build;
  }

  info.architecture = architecture_name();
  return info;
}

}  // namespace hwspec
