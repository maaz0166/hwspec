#include "hwspec/collectors.hpp"
// Non-Windows builds are for portable formatter / CLI tests only.
namespace hwspec {
CpuInfo collect_cpu(bool){return {};}
MemInfo collect_mem(bool){return {};}
OsInfo collect_os(bool){return {};}
GpuInfo collect_gpu(bool){return {};}
DiskInfo collect_disk(bool){return {};}
DiskInfo collect_disk_with_health(bool detail, bool allow_admin){
  auto disk=collect_disk(detail);
  if(detail && allow_admin)disk.health_status=DiskHealthStatus::unknown;
  return disk;
}
NetInfo collect_net(bool){return {};}
BatteryInfo collect_battery(bool){return {};}
}
