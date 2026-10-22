#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// A missing value is not zero: formatters display Unknown and JSON emits null.
namespace hwspec {

struct CpuInfo {
  std::optional<std::string> name;
  std::optional<unsigned> physical_cores, logical_cores;
  std::optional<double> base_clock_ghz;
  std::optional<std::string> vendor, extensions;
  std::optional<unsigned> family, model, stepping;
  std::optional<double> max_boost_clock_ghz, current_clock_ghz;
  std::optional<std::uint64_t> l1_cache_bytes, l2_cache_bytes, l3_cache_bytes;
  std::optional<bool> virtualization_enabled;
};

struct MemoryModule {
  std::optional<std::string> slot, type, manufacturer;
  std::optional<std::uint64_t> size_bytes;
  std::optional<unsigned> speed_mhz;
};
struct MemInfo {
  std::optional<std::uint64_t> total_bytes, available_bytes;
  std::vector<MemoryModule> modules;
  std::optional<std::uint64_t> page_file_bytes, page_file_used_bytes;
};

struct OsInfo {
  std::optional<std::string> name, version, build, architecture;
  std::optional<std::uint64_t> uptime_seconds;
  std::optional<std::string> install_date, computer_name;
  std::optional<unsigned> build_revision;
};

struct GpuAdapter {
  std::optional<std::string> name, driver_version, resolution;
  std::optional<std::uint64_t> dedicated_bytes, shared_bytes;
  std::optional<unsigned> vendor_id, device_id, refresh_hz;
};
struct GpuInfo { std::vector<GpuAdapter> adapters; };

struct DiskDrive {
  std::optional<std::string> letter, file_system, model, media_type, bus, firmware;
  std::optional<std::uint64_t> size_bytes, free_bytes;
};
enum class DiskHealthStatus { requires_admin, unknown, available };
struct DiskHealthDevice {
  std::optional<std::string> instance_name;
  std::optional<bool> failure_predicted;
};
struct DiskInfo {
  std::vector<DiskDrive> drives;
  DiskHealthStatus health_status = DiskHealthStatus::requires_admin;
  // WMI physical-device instances; never guessed from logical volume order.
  std::vector<DiskHealthDevice> health_devices;
};

struct NetAdapter {
  std::optional<std::string> name, status, mac, gateway;
  std::vector<std::string> addresses, dns_servers;
  std::optional<std::uint64_t> link_speed_bps;
};
struct NetInfo { std::vector<NetAdapter> adapters; };

struct BatteryInfo {
  std::optional<bool> present;  // null if Windows cannot determine presence
  std::optional<unsigned> charge_percent, cycle_count;
  std::optional<std::string> power_state;
  std::optional<double> design_capacity_wh, full_charge_capacity_wh;
};

}  // namespace hwspec
