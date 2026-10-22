#include "hwspec/collectors.hpp"
// Non-Windows builds are for portable formatter / CLI tests only.
namespace hwspec {
CpuInfo collect_cpu(bool){return {};}
MemInfo collect_mem(bool){return {};}
OsInfo collect_os(bool){return {};}
GpuInfo collect_gpu(bool){return {};}
DiskInfo collect_disk(bool){return {};}
NetInfo collect_net(bool){return {};}
BatteryInfo collect_battery(bool){return {};}
}
