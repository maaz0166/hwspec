#include "hwspec/json.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace hwspec {
std::string json_quote(const std::string& value) {
  std::string out="\"";
  const char* hex="0123456789abcdef";
  for(unsigned char c:value){
    switch(c){
      case '"':out+="\\\"";break;
      case '\\':out+="\\\\";break;
      case '\b':out+="\\b";break;
      case '\f':out+="\\f";break;
      case '\n':out+="\\n";break;
      case '\r':out+="\\r";break;
      case '\t':out+="\\t";break;
      default:
        if(c<0x20){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
        else out+=static_cast<char>(c);
    }
  }
  return out+"\"";
}
namespace {
template<class T> std::string j(const std::optional<T>& v){return v?std::to_string(*v):"null";}
std::string j(const std::optional<std::string>& v){return v?json_quote(*v):"null";}
std::string j(const std::optional<bool>& v){return v?(*v?"true":"false"):"null";}
std::string j(const std::optional<double>& v){
  if(!v||!std::isfinite(*v))return "null";
  std::ostringstream s;s.precision(8);s<<*v;return s.str();
}
struct Object {
  std::string result="{";bool first=true;
  void add(const std::string& key,const std::string& raw){if(!first)result+=",";first=false;result+=json_quote(key)+":"+raw;}
  std::string done(){return result+"}";}
};
std::string jstrings(const std::vector<std::string>& a){
  std::string out="[";for(std::size_t k=0;k<a.size();++k){if(k)out+=",";out+=json_quote(a[k]);}return out+"]";
}
} // namespace

std::string json_cpu(const CpuInfo& i,bool d){
  Object r;r.add("name",j(i.name));r.add("physical_cores",j(i.physical_cores));
  r.add("logical_cores",j(i.logical_cores));r.add("base_clock_ghz",j(i.base_clock_ghz));
  if(d){
    r.add("vendor",j(i.vendor));r.add("max_boost_clock_ghz",j(i.max_boost_clock_ghz));
    r.add("current_clock_ghz",j(i.current_clock_ghz));
    r.add("l1_cache_bytes",j(i.l1_cache_bytes));r.add("l2_cache_bytes",j(i.l2_cache_bytes));
    r.add("l3_cache_bytes",j(i.l3_cache_bytes));r.add("family",j(i.family));
    r.add("model",j(i.model));r.add("stepping",j(i.stepping));
    r.add("extensions",j(i.extensions));r.add("virtualization_enabled",j(i.virtualization_enabled));
  }return r.done();
}
std::string json_mem(const MemInfo& i,bool d){
  Object r;r.add("total_bytes",j(i.total_bytes));r.add("available_bytes",j(i.available_bytes));
  std::optional<std::uint64_t> used;
  if(i.total_bytes && i.available_bytes)used=*i.total_bytes-std::min(*i.total_bytes,*i.available_bytes);
  r.add("used_bytes",j(used));
  if(d){
    std::string a="[";for(std::size_t n=0;n<i.modules.size();++n){
      if(n)a+=",";
      Object m;const auto& x=i.modules[n];
      m.add("slot",j(x.slot));m.add("size_bytes",j(x.size_bytes));m.add("type",j(x.type));
      m.add("speed_mhz",j(x.speed_mhz));m.add("manufacturer",j(x.manufacturer));a+=m.done();
    }a+="]";r.add("modules",a);r.add("page_file_bytes",j(i.page_file_bytes));
    r.add("page_file_used_bytes",j(i.page_file_used_bytes));
  }return r.done();
}
std::string json_os(const OsInfo& i,bool d){
  Object r;r.add("name",j(i.name));r.add("version",j(i.version));
  r.add("build",j(i.build));r.add("architecture",j(i.architecture));
  if(d){r.add("uptime_seconds",j(i.uptime_seconds));r.add("install_date",j(i.install_date));
    r.add("build_revision",j(i.build_revision));r.add("computer_name",j(i.computer_name));}
  return r.done();
}
std::string json_gpu(const GpuInfo& i,bool d){
  std::string a="[";const auto count=d?i.adapters.size():std::min(std::size_t(1),i.adapters.size());
  for(std::size_t n=0;n<count;++n){
    if(n)a+=",";
    const auto& x=i.adapters[n];Object o;
    o.add("name",j(x.name));o.add("dedicated_bytes",j(x.dedicated_bytes));
    if(d){o.add("shared_bytes",j(x.shared_bytes));o.add("vendor_id",j(x.vendor_id));
      o.add("device_id",j(x.device_id));o.add("driver_version",j(x.driver_version));
      o.add("resolution",j(x.resolution));o.add("refresh_hz",j(x.refresh_hz));}
    a+=o.done();
  }return "{\"adapters\":"+a+"]}";
}
std::string json_disk(const DiskInfo& i,bool d){
  std::string a="[";for(std::size_t n=0;n<i.drives.size();++n){
    if(n)a+=",";
    const auto& x=i.drives[n];Object o;
    o.add("drive",j(x.letter));o.add("size_bytes",j(x.size_bytes));o.add("free_bytes",j(x.free_bytes));
    if(d){o.add("model",j(x.model));o.add("media_type",j(x.media_type));o.add("bus",j(x.bus));
      o.add("firmware",j(x.firmware));o.add("file_system",j(x.file_system));}
    a+=o.done();
  }a+="]";Object r;r.add("drives",a);
  if(d){
    Object health;
    health.add("status",json_quote(i.health_status==DiskHealthStatus::requires_admin?"requires_admin":
                                  i.health_status==DiskHealthStatus::available?"available":"unknown"));
    std::string devices="[";
    for(std::size_t n=0;n<i.health_devices.size();++n){
      if(n)devices+=",";
      const auto& device=i.health_devices[n];Object entry;
      entry.add("instance_name",j(device.instance_name));
      entry.add("failure_predicted",j(device.failure_predicted));
      devices+=entry.done();
    }
    health.add("devices",devices+"]");
    r.add("disk_health",health.done());
  }
  return r.done();
}
std::string json_net(const NetInfo& i,bool d){
  std::string a="[";for(std::size_t n=0;n<i.adapters.size();++n){
    if(n)a+=",";
    const auto& x=i.adapters[n];Object o;
    o.add("name",j(x.name));o.add("addresses",jstrings(x.addresses));
    if(d){o.add("link_speed_bps",j(x.link_speed_bps));o.add("status",j(x.status));
      o.add("mac",j(x.mac));o.add("gateway",j(x.gateway));o.add("dns_servers",jstrings(x.dns_servers));}
    a+=o.done();
  }return "{\"adapters\":"+a+"]}";
}
std::string json_battery(const BatteryInfo& i,bool d){
  Object r;r.add("present",j(i.present));
  r.add("charge_percent",j(i.charge_percent));r.add("power_state",j(i.power_state));
  if(d){r.add("design_capacity_wh",j(i.design_capacity_wh));
    r.add("full_charge_capacity_wh",j(i.full_charge_capacity_wh));
    std::optional<double> health;
    if(i.design_capacity_wh && i.full_charge_capacity_wh && *i.design_capacity_wh>0)
      health=100.0**i.full_charge_capacity_wh / *i.design_capacity_wh;
    r.add("health_percent",j(health));r.add("cycle_count",j(i.cycle_count));}
  return r.done();
}
} // namespace hwspec
