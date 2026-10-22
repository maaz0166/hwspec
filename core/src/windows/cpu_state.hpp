#pragma once
#include <cstdint>
#include <optional>

namespace hwspec::win {
// Each RelationCache record describes one cache, including shared caches once.
inline void accumulate_cache_bytes(std::optional<std::uint64_t>& total, std::uint32_t bytes) {
  if (bytes) total = total.value_or(0) + bytes;
}
// A negative availability flag under a hypervisor cannot establish BIOS state.
inline std::optional<bool> firmware_virtualization_state(bool os_available, bool hypervisor_present) {
  if (os_available) return true;
  if (hypervisor_present) return std::nullopt;
  return false;
}
} // namespace hwspec::win
