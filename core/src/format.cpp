#include "hwspec/format.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace hwspec {
namespace {
const char* const kUnknown="Unknown";
std::string show(const std::optional<std::string>& v){return v && !v->empty()? *v:kUnknown;}
template<class T> std::string number(const std::optional<T>& v) {return v?std::to_string(*v):kUnknown;}
std::string bytes(const std::optional<std::uint64_t>& v){return v?format_bytes(*v):kUnknown;}
std::string ghz(const std::optional<double>& v){return v?format_ghz(*v):kUnknown;}
std::string wh(const std::optional<double>& v){
  if(!v) return kUnknown;
  char s[40];std::snprintf(s,sizeof s,"%.2f Wh",*v);return s;
}
std::string hex4(const std::optional<unsigned>& value) {
  if(!value) return kUnknown;
  char s[16];std::snprintf(s,sizeof s,"0x%04X",*value);return s;
}
std::string comma_join(const std::vector<std::string>& items){
  if(items.empty())return kUnknown;
  std::string out;
  for(const auto& item:items){if(!out.empty())out+=", ";out+=item;}
  return out;
}
} // namespace

std::string format_bytes(std::uint64_t n){
  static const char* units[]={"B","KB","MB","GB","TB","PB"};
  if(n<1024)return std::to_string(n)+" B";
  double value=static_cast<double>(n);int unit=0;
  while(value>=1024 && unit<5){value/=1024;++unit;}
  char s[48];std::snprintf(s,sizeof s,"%.1f %s",value,units[unit]);return s;
}
std::string format_ghz(double n){char s[32];std::snprintf(s,sizeof s,"%.2f GHz",n);return s;}

SectionReport format_cpu(const CpuInfo& i,bool detail){
  SectionReport r{"CPU",{{"Name",show(i.name)}}};
  std::string cores=kUnknown;
  if(i.physical_cores&&i.logical_cores)
    cores=std::to_string(*i.physical_cores)+" physical, "+std::to_string(*i.logical_cores)+" logical";
  else if(i.physical_cores)cores=std::to_string(*i.physical_cores)+" physical";
  else if(i.logical_cores)cores=std::to_string(*i.logical_cores)+" logical";
  r.fields.push_back({"Cores",cores});
  r.fields.push_back({"Base clock",ghz(i.base_clock_ghz)});
  if(detail){
    r.fields.push_back({"Vendor",show(i.vendor)});
    r.fields.push_back({"Max boost clock",ghz(i.max_boost_clock_ghz)});
    r.fields.push_back({"Current clock",ghz(i.current_clock_ghz)});
    r.fields.push_back({"Cache L1",bytes(i.l1_cache_bytes)});
    r.fields.push_back({"Cache L2",bytes(i.l2_cache_bytes)});
    r.fields.push_back({"Cache L3",bytes(i.l3_cache_bytes)});
    std::string family=i.family?std::to_string(*i.family):kUnknown;
    std::string model=i.model?std::to_string(*i.model):kUnknown;
    r.fields.push_back({"Family / model",family+", "+model+", stepping "+number(i.stepping)});
    r.fields.push_back({"Extensions",show(i.extensions)});
    r.fields.push_back({"Virtualization",i.virtualization_enabled?(*i.virtualization_enabled?"Enabled":"Disabled"):kUnknown});
  }
  return r;
}

SectionReport format_mem(const MemInfo& i,bool detail){
  SectionReport r{"Memory",{{"Total",bytes(i.total_bytes)}}};
  if(i.total_bytes && i.available_bytes){
    const auto available=std::min(*i.total_bytes,*i.available_bytes);
    const auto used=*i.total_bytes-available;
    const auto percent=*i.total_bytes?static_cast<unsigned>(std::round(100.0*static_cast<double>(used)/static_cast<double>(*i.total_bytes))):0;
    r.fields.push_back({"Used",format_bytes(used)+" ("+std::to_string(percent)+"%)"});
    r.fields.push_back({"Available",format_bytes(available)});
  }else{
    r.fields.push_back({"Used",kUnknown});
    r.fields.push_back({"Available",i.total_bytes? kUnknown:bytes(i.available_bytes)});
  }
  if(detail){
    if(i.modules.empty())r.fields.push_back({"RAM modules",kUnknown});
    for(std::size_t n=0;n<i.modules.size();++n){
      const auto& m=i.modules[n];
      r.fields.push_back({"Module "+std::to_string(n+1),show(m.slot)+" | "+bytes(m.size_bytes)+" | "+show(m.type)+" | "+
                          (m.speed_mhz?std::to_string(*m.speed_mhz)+" MHz":kUnknown)+" | "+show(m.manufacturer)});
    }
    r.fields.push_back({"Page file size",bytes(i.page_file_bytes)});
    r.fields.push_back({"Page file used",bytes(i.page_file_used_bytes)});
  }
  return r;
}

SectionReport format_os(const OsInfo& i,bool detail){
  SectionReport r{"OS",{{"Name",show(i.name)},{"Version",show(i.version)},
                        {"Build",show(i.build)},{"Architecture",show(i.architecture)}}};
  if(detail){
    r.fields.push_back({"Uptime",i.uptime_seconds?std::to_string(*i.uptime_seconds/86400)+"d "+
                       std::to_string((*i.uptime_seconds%86400)/3600)+"h "+
                       std::to_string((*i.uptime_seconds%3600)/60)+"m":kUnknown});
    r.fields.push_back({"Install date",show(i.install_date)});
    r.fields.push_back({"Build revision",number(i.build_revision)});
    r.fields.push_back({"Computer name",show(i.computer_name)});
  }
  return r;
}

SectionReport format_gpu(const GpuInfo& i,bool detail){
  SectionReport r{"GPU",{}};
  if(i.adapters.empty()) {r.fields.push_back({"Adapter","Unknown"});return r;}
  const std::size_t limit=detail?i.adapters.size():1;
  for(std::size_t n=0;n<limit;++n){
    const auto& a=i.adapters[n];const std::string prefix=limit>1?std::to_string(n+1)+" ":"";
    r.fields.push_back({prefix+"Name",show(a.name)});
    r.fields.push_back({prefix+"Dedicated memory",bytes(a.dedicated_bytes)});
    if(detail){
      r.fields.push_back({prefix+"Shared memory",bytes(a.shared_bytes)});
      r.fields.push_back({prefix+"Vendor ID",hex4(a.vendor_id)});
      r.fields.push_back({prefix+"Device ID",hex4(a.device_id)});
      r.fields.push_back({prefix+"Driver version",show(a.driver_version)});
      r.fields.push_back({prefix+"Resolution",show(a.resolution)});
      r.fields.push_back({prefix+"Refresh rate",a.refresh_hz?std::to_string(*a.refresh_hz)+" Hz":kUnknown});
    }
  }
  return r;
}

SectionReport format_disk(const DiskInfo& i,bool detail){
  SectionReport r{"Disk",{}};
  if(i.drives.empty())r.fields.push_back({"Drives","Unknown"});
  for(const auto& d:i.drives){
    const auto name=show(d.letter);
    r.fields.push_back({name+" size",bytes(d.size_bytes)});
    r.fields.push_back({name+" free",bytes(d.free_bytes)});
    if(detail){
      r.fields.push_back({name+" model",show(d.model)});
      r.fields.push_back({name+" type",show(d.media_type)});
      r.fields.push_back({name+" bus",show(d.bus)});
      r.fields.push_back({name+" firmware",show(d.firmware)});
      r.fields.push_back({name+" file system",show(d.file_system)});
    }
  }
  if(detail){
    r.fields.push_back({"Disk health",i.health_status==DiskHealthStatus::requires_admin?"requires admin":
                                   i.health_status==DiskHealthStatus::available?"SMART failure prediction":"Unknown"});
    for(std::size_t n=0;n<i.health_devices.size();++n){
      const auto& device=i.health_devices[n];
      r.fields.push_back({"SMART "+std::to_string(n+1)+" device",show(device.instance_name)});
      r.fields.push_back({"SMART "+std::to_string(n+1)+" prediction",device.failure_predicted?
                         (*device.failure_predicted?"Failure predicted":"No failure predicted"):"Unknown"});
    }
  }
  return r;
}

SectionReport format_net(const NetInfo& i,bool detail){
  SectionReport r{"Network",{}};
  if(i.adapters.empty()){r.fields.push_back({"Adapters","Unknown"});return r;}
  for(std::size_t n=0;n<i.adapters.size();++n){
    const auto& a=i.adapters[n];const auto prefix=std::to_string(n+1)+" ";
    r.fields.push_back({prefix+"Adapter",show(a.name)});
    r.fields.push_back({prefix+"Addresses",comma_join(a.addresses)});
    if(detail){
      r.fields.push_back({prefix+"Link speed",a.link_speed_bps?std::to_string(*a.link_speed_bps/1000000)+" Mbps":kUnknown});
      r.fields.push_back({prefix+"Status",show(a.status)});
      r.fields.push_back({prefix+"MAC",show(a.mac)});
      r.fields.push_back({prefix+"Gateway",show(a.gateway)});
      r.fields.push_back({prefix+"DNS servers",comma_join(a.dns_servers)});
    }
  }
  return r;
}

SectionReport format_battery(const BatteryInfo& i,bool detail){
  SectionReport r{"Battery",{}};
  if(!i.present){r.fields.push_back({"Status","Unknown"});return r;}
  if(!*i.present){r.fields.push_back({"Status","No battery detected"});return r;}
  r.fields.push_back({"Charge",i.charge_percent?std::to_string(*i.charge_percent)+"%":kUnknown});
  r.fields.push_back({"State",show(i.power_state)});
  if(detail){
    r.fields.push_back({"Design capacity",wh(i.design_capacity_wh)});
    r.fields.push_back({"Full charge",wh(i.full_charge_capacity_wh)});
    if(i.design_capacity_wh && i.full_charge_capacity_wh && *i.design_capacity_wh>0) {
      char s[40];std::snprintf(s,sizeof s,"%.1f%%",100.0**i.full_charge_capacity_wh / *i.design_capacity_wh);
      r.fields.push_back({"Health",s});
    } else r.fields.push_back({"Health",kUnknown});
    r.fields.push_back({"Cycle count",number(i.cycle_count)});
  }
  return r;
}
} // namespace hwspec
