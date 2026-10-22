#include "check.hpp"
#include "hwspec/format.hpp"
#include "hwspec/json.hpp"
#include "hwspec/sections.hpp"
#include "../core/src/windows/cpu_state.hpp"
#include "../core/src/windows/memory_state.hpp"
#include <string>

using namespace hwspec;
namespace {
bool has(const std::string& content,const std::string& text){return content.find(text)!=std::string::npos;}
void section_registry(){
  const auto& sections=default_sections();
  CHECK_EQ(sections.size(),static_cast<std::size_t>(7));
  for(const auto& name:{"cpu","mem","os","gpu","disk","net","battery"}){
    bool found=false;
    for(const auto& s:sections)if(s.flag==name){
      found=true;CHECK(!s.requires_admin);CHECK(static_cast<bool>(s.collect_detail));CHECK(static_cast<bool>(s.collect_json));
      CHECK(s.detail_requires_admin==(s.flag=="disk"));
      if(s.detail_requires_admin){CHECK(static_cast<bool>(s.collect_detail_admin));CHECK(static_cast<bool>(s.collect_json_admin));}
    }
    CHECK(found);
  }
}
void cpu_details(){
  CpuInfo c;c.name="Test CPU";c.base_clock_ghz=2.0;c.max_boost_clock_ghz=3.4;
  c.virtualization_enabled=false;c.l3_cache_bytes=8*1024*1024;
  const auto r=format_cpu(c,true);
  CHECK_EQ(r.fields[0].value,std::string("Test CPU"));
  CHECK_EQ(r.fields[4].value,std::string("3.40 GHz"));
  CHECK_EQ(r.fields[7].value,std::string("Unknown")); // L1 not reported
  const std::string j=json_cpu(c,true);
  CHECK(has(j,"\"max_boost_clock_ghz\":3.4"));
  CHECK(has(j,"\"virtualization_enabled\":false"));
  CHECK(has(j,"\"l1_cache_bytes\":null"));
  CHECK(!has(json_cpu(c,false),"virtualization_enabled"));
}
void virtualization_with_hypervisor(){
  CHECK(win::firmware_virtualization_state(true,false)==std::optional<bool>(true));
  CHECK(win::firmware_virtualization_state(true,true)==std::optional<bool>(true));
  CHECK(win::firmware_virtualization_state(false,false)==std::optional<bool>(false));
  CpuInfo cpu;
  cpu.virtualization_enabled=win::firmware_virtualization_state(false,true);
  CHECK(!cpu.virtualization_enabled.has_value());
  CHECK(has(json_cpu(cpu,true),"\"virtualization_enabled\":null"));
  const auto report=format_cpu(cpu,true);
  CHECK_EQ(report.fields.back().label,std::string("Virtualization"));
  CHECK_EQ(report.fields.back().value,std::string("Unknown"));
}
void memory_details(){
  MemInfo m;m.total_bytes=16ULL*1024*1024*1024;m.available_bytes=8ULL*1024*1024*1024;
  MemoryModule module;module.slot="DIMM 0";module.size_bytes=8ULL*1024*1024*1024;
  module.type="DDR4";module.speed_mhz=3200;m.modules.push_back(module);
  CHECK(has(render_text({format_mem(m,true)}),"DDR4"));
  const auto j=json_mem(m,true);
  CHECK(has(j,"\"modules\":[{"));CHECK(has(j,"\"used_bytes\":8589934592"));
}
void hardware_detail_sources(){
  CpuInfo cpu;
  win::accumulate_cache_bytes(cpu.l1_cache_bytes,0);
  CHECK(!cpu.l1_cache_bytes);
  // Four cores, each with separate 32 KiB data and instruction caches.
  for(int i=0;i<8;++i)win::accumulate_cache_bytes(cpu.l1_cache_bytes,32*1024);
  for(int i=0;i<4;++i)win::accumulate_cache_bytes(cpu.l2_cache_bytes,256*1024);
  win::accumulate_cache_bytes(cpu.l3_cache_bytes,6*1024*1024); // shared once
  CHECK(cpu.l1_cache_bytes==std::optional<std::uint64_t>(256*1024));
  CHECK(cpu.l2_cache_bytes==std::optional<std::uint64_t>(1024*1024));
  CHECK(cpu.l3_cache_bytes==std::optional<std::uint64_t>(6*1024*1024));
  CHECK(has(json_cpu(cpu,true),"\"l1_cache_bytes\":262144"));
  CHECK(!win::configured_memory_speed(std::nullopt));
  CHECK(!win::configured_memory_speed(0));
  CHECK(!win::configured_memory_speed(1ULL<<32));
  CHECK(win::configured_memory_speed(2400)==std::optional<unsigned>(2400));
}
void gpu_and_disks(){
  GpuInfo g;g.adapters.push_back(GpuAdapter{});
  CHECK(has(json_gpu(g,false),"\"dedicated_bytes\":null"));
  DiskInfo d;DiskDrive disk;disk.letter="C:\\";disk.size_bytes=1000;d.drives.push_back(disk);
  CHECK(has(json_disk(d,true),"\"status\":\"requires_admin\""));
  CHECK(has(render_text({format_disk(d,true)}),"requires admin"));
  NetInfo n;NetAdapter adapter;adapter.addresses={"10.0.0.5","fe80::1"};n.adapters.push_back(adapter);
  CHECK(has(json_net(n,true),"fe80::1"));
}
void batteries(){
  BatteryInfo b;
  CHECK(has(render_text({format_battery(b,true)}),"Unknown"));
  b.present=false;
  CHECK(has(render_text({format_battery(b,true)}),"No battery detected"));
  b.present=true;b.charge_percent=72;b.design_capacity_wh=50.0;b.full_charge_capacity_wh=40.0;
  b.cycle_count=300;
  CHECK(has(render_text({format_battery(b,true)}),"80.0%"));
  CHECK(has(json_battery(b,true),"\"health_percent\":80"));
  b.design_capacity_wh=0.0;
  CHECK(has(json_battery(b,true),"\"health_percent\":null"));
  CHECK(!has(json_battery(b,false),"health_percent"));
}
void json_escaping(){
  CHECK_EQ(json_quote("ab\"\\\n\t"),std::string("\"ab\\\"\\\\\\n\\t\""));
  CHECK_EQ(json_quote(std::string(1,'\x01')),std::string("\"\\u0001\""));
}
} // namespace
int main(){
  section_registry();cpu_details();virtualization_with_hypervisor();memory_details();hardware_detail_sources();gpu_and_disks();batteries();json_escaping();
  return finish();
}
