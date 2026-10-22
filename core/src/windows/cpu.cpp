#include <string>
#include <array>
#include <cstring>
#include <windows.h>
#include <powrprof.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#include <vector>

#include "hwspec/collectors.hpp"
#include "registry.hpp"
#include "cpu_state.hpp"

namespace hwspec {

namespace {

// Windows stores one key per logical processor; processor 0 is always there.
const char* const kCpuKey = R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)";

std::string trim(const std::string& text) {
  const char* const whitespace = " \t\r\n";
  const std::size_t first = text.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = text.find_last_not_of(whitespace);
  return text.substr(first, last - first + 1);
}

// Physical cores = how many "processor core" records Windows reports.
std::optional<unsigned> count_physical_cores() {
  DWORD length = 0;
  // The first call fails on purpose and tells us how big the buffer must be.
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || length == 0) {
    return std::nullopt;
  }

  std::vector<char> buffer(length);
  if (!GetLogicalProcessorInformationEx(
          RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length)) {
    return std::nullopt;
  }

  // The buffer holds several records of different sizes, one after another.
  unsigned count = 0;
  DWORD offset = 0;
  while (offset < length) {
    const auto* record = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);
    if (record->Size == 0) {
      break;
    }
    if (record->Relationship == RelationProcessorCore) {
      ++count;
    }
    offset += record->Size;
  }
  if (count == 0) {
    return std::nullopt;
  }
  return count;
}

// CPUID provides hardware capability, not Windows utilization metrics.
std::array<unsigned,4> cpuid(unsigned leaf,unsigned subleaf=0){
  std::array<unsigned,4> result{};
#if defined(_MSC_VER)
  int regs[4]{};__cpuidex(regs,static_cast<int>(leaf),static_cast<int>(subleaf));
  for(int i=0;i<4;++i)result[static_cast<std::size_t>(i)]=static_cast<unsigned>(regs[i]);
#else
  __cpuid_count(leaf,subleaf,result[0],result[1],result[2],result[3]);
#endif
  return result;
}
void read_cpuid(CpuInfo& info){
  const auto maxleaf=cpuid(0);
  char vendor[13]{};
  std::memcpy(vendor,&maxleaf[1],4);std::memcpy(vendor+4,&maxleaf[3],4);
  std::memcpy(vendor+8,&maxleaf[2],4);
  if(vendor[0])info.vendor=vendor;
  if(maxleaf[0]<1)return;
  const auto one=cpuid(1);
  unsigned family=(one[0]>>8)&15, model=(one[0]>>4)&15;
  if(family==15)family+=(one[0]>>20)&255;
  if(family==6 || family==15)model+=((one[0]>>16)&15)<<4;
  info.family=family;info.model=model;info.stepping=one[0]&15;
  std::vector<std::string> features;
  auto add=[&features](bool yes,const char* label){if(yes)features.emplace_back(label);};
  add((one[3]&(1u<<25))!=0,"SSE");add((one[3]&(1u<<26))!=0,"SSE2");
  add((one[2]&(1u<<0))!=0,"SSE3");add((one[2]&(1u<<9))!=0,"SSSE3");
  add((one[2]&(1u<<19))!=0,"SSE4.1");add((one[2]&(1u<<20))!=0,"SSE4.2");
  add((one[2]&(1u<<28))!=0,"AVX");add((one[2]&(1u<<25))!=0,"AES");
  add((one[2]&(1u<<5))!=0,"VMX");
  if(maxleaf[0]>=7){
    const auto seven=cpuid(7);
    add((seven[1]&(1u<<5))!=0,"AVX2");
    add((seven[1]&(1u<<16))!=0,"AVX-512F");
    add((seven[1]&(1u<<3))!=0,"BMI1");
    add((seven[1]&(1u<<8))!=0,"BMI2");
  }
  if(!features.empty()){
    std::string joined;
    for(const auto& feature:features){if(!joined.empty())joined+=", ";joined+=feature;}
    info.extensions=joined;
  }
  // Leaf 0x16 reports base and maximum (MHz) on CPUs that implement it.
  if(maxleaf[0]>=0x16){
    const auto frequency=cpuid(0x16);
    if(frequency[0]>0)info.base_clock_ghz=frequency[0]/1000.0;
    if(frequency[1]>0)info.max_boost_clock_ghz=frequency[1]/1000.0;
  }
}
void read_caches(CpuInfo& info){
  DWORD length=0;
  GetLogicalProcessorInformationEx(RelationCache,nullptr,&length);
  if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER || !length)return;
  std::vector<char> buffer(length);
  if(!GetLogicalProcessorInformationEx(RelationCache,
      reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),&length))return;
  for(DWORD offset=0;offset<length;){
    const auto* record=reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data()+offset);
    if(!record->Size || record->Size>length-offset)break;
    if(record->Relationship==RelationCache){
      const auto& cache=record->Cache;
      auto* target=cache.Level==1?&info.l1_cache_bytes:cache.Level==2?&info.l2_cache_bytes:
                   cache.Level==3?&info.l3_cache_bytes:nullptr;
      if(target)win::accumulate_cache_bytes(*target,cache.CacheSize);
    }
    offset+=record->Size;
  }
}
void read_current_clock(CpuInfo& info){
  // Documented ProcessorInformation layout (not declared by every Windows SDK).
  struct ProcessorPowerInformation {
    ULONG Number, MaxMhz, CurrentMhz, MhzLimit, MaxIdleState, CurrentIdleState;
  };
  const DWORD count=GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if(!count || count>4096)return;
  std::vector<ProcessorPowerInformation> processors(count);
  if(CallNtPowerInformation(ProcessorInformation,nullptr,0,processors.data(),
                            static_cast<ULONG>(processors.size()*sizeof(processors[0])))==0){
    std::uint64_t total=0;unsigned actual=0;
    for(const auto& p:processors){if(p.CurrentMhz){total+=p.CurrentMhz;++actual;}}
    if(actual)info.current_clock_ghz=static_cast<double>(total)/actual/1000.0;
  }
}

}  // namespace

CpuInfo collect_cpu(bool detail) {
  CpuInfo info;
  if (const auto name = win::reg_string(kCpuKey, "ProcessorNameString")) {
    const std::string cleaned = trim(*name);
    if (!cleaned.empty()) info.name = cleaned;
  }
  info.physical_cores = count_physical_cores();
  const DWORD logical = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if(logical>0)info.logical_cores=static_cast<unsigned>(logical);
  read_cpuid(info);
  if(!info.base_clock_ghz){
    if(const auto mhz=win::reg_dword(kCpuKey,"~MHz")){
      if(*mhz>0) info.base_clock_ghz=static_cast<double>(*mhz)/1000.0;
    }
  }
  if(detail){
    read_caches(info);read_current_clock(info);
#ifdef PF_VIRT_FIRMWARE_ENABLED
    const bool hypervisor_present=cpuid(0)[0]>=1 && (cpuid(1)[2]&(1u<<31))!=0;
    info.virtualization_enabled=win::firmware_virtualization_state(
        IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED)!=FALSE,hypervisor_present);
#endif
  }
  return info;
}
}  // namespace hwspec
