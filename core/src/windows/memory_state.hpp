#pragma once
#include <cstdint>
#include <limits>
#include <optional>

namespace hwspec::win {
// Use the configured value, not the module's rated Speed; zero means unknown.
inline std::optional<unsigned> configured_memory_speed(std::optional<std::uint64_t> value) {
  if (!value || !*value || *value > (std::numeric_limits<unsigned>::max)()) return std::nullopt;
  return static_cast<unsigned>(*value);
}
} // namespace hwspec::win
