#include "hwspec/collectors.hpp"
#include "registry.hpp"
#include "wmi.hpp"
#include "memory_state.hpp"
#include <cstdlib>
#include <limits>
#include <string>

namespace hwspec {
namespace {
std::optional<std::uint64_t> as_u64(const win::WmiRow& row,const std::string& key){
  const auto it=row.find(key);
  if(it==row.end())return std::nullopt;
  try{return std::stoull(it->second);}catch(...){return std::nullopt;}
}
std::optional<std::string> as_string(const win::WmiRow& row,const std::string& key){
  const auto it=row.find(key);return it!=row.end() && !it->second.empty()?std::optional<std::string>(it->second):std::nullopt;
}
std::optional<std::string> ram_type(std::uint64_t type){
  switch(type){case 20:return "DDR";case 21:return "DDR2";case 24:return "DDR3";
    case 26:return "DDR4";case 34:return "DDR5";case 30:return "LPDDR4";
    case 35:return "LPDDR5";case 29:return "LPDDR3";default:return std::nullopt;}
}
} // namespace
MemInfo collect_mem(bool detail){
  MemInfo info;
  MEMORYSTATUSEX status{};status.dwLength=sizeof status;
  if(GlobalMemoryStatusEx(&status)){
    info.total_bytes=static_cast<std::uint64_t>(status.ullTotalPhys);
    info.available_bytes=static_cast<std::uint64_t>(status.ullAvailPhys);
  }
  if(!detail)return info;
  for(const auto& row:win::wmi_query(L"SELECT DeviceLocator,Capacity,SMBIOSMemoryType,MemoryType,ConfiguredClockSpeed,Manufacturer FROM Win32_PhysicalMemory",
         {"DeviceLocator","Capacity","SMBIOSMemoryType","MemoryType","ConfiguredClockSpeed","Manufacturer"})){
    MemoryModule module;
    module.slot=as_string(row,"DeviceLocator");module.size_bytes=as_u64(row,"Capacity");
    module.manufacturer=as_string(row,"Manufacturer");
    const auto type=as_u64(row,"SMBIOSMemoryType");
    const auto oldtype=as_u64(row,"MemoryType");
    if(type)module.type=ram_type(*type);
    if(!module.type && oldtype)module.type=ram_type(*oldtype);
    module.speed_mhz=win::configured_memory_speed(as_u64(row,"ConfiguredClockSpeed"));
    info.modules.push_back(std::move(module));
  }
  std::uint64_t total_allocated=0,total_used=0;bool has_pagefile=false;
  for(const auto& row:win::wmi_query(L"SELECT AllocatedBaseSize,CurrentUsage FROM Win32_PageFileUsage",
                                {"AllocatedBaseSize","CurrentUsage"})){
    const auto allocated=as_u64(row,"AllocatedBaseSize"),used=as_u64(row,"CurrentUsage");
    if(allocated){total_allocated+=*allocated;has_pagefile=true;}
    if(used)total_used+=*used;
  }
  if(has_pagefile){info.page_file_bytes=total_allocated*1024ULL*1024ULL;
    info.page_file_used_bytes=total_used*1024ULL*1024ULL;}
  return info;
}
} // namespace hwspec
