#pragma once
#include <optional>

namespace hwspec::win {
// A negative availability flag under a hypervisor cannot establish BIOS state.
inline std::optional<bool> firmware_virtualization_state(bool os_available, bool hypervisor_present) {
  if (os_available) return true;
  if (hypervisor_present) return std::nullopt;
  return false;
}
} // namespace hwspec::win
