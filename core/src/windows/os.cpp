#include <cstdlib>
#include <ctime>
#include <string>
#include "hwspec/collectors.hpp"
#include "registry.hpp"

namespace hwspec {
namespace {
const char* const kVersionKey=R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)";
std::string fix_windows_11_name(const std::string& name,unsigned long build){
  const std::string prefix="Windows 10";
  if(build>=22000 && name.compare(0,prefix.size(),prefix)==0)return "Windows 11"+name.substr(prefix.size());
  return name;
}
std::optional<std::string> architecture_name(){
  SYSTEM_INFO system_info{};GetNativeSystemInfo(&system_info);
  switch(system_info.wProcessorArchitecture){
    case PROCESSOR_ARCHITECTURE_AMD64:return "x64";
    case PROCESSOR_ARCHITECTURE_ARM64:return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:return "x86";
    case PROCESSOR_ARCHITECTURE_ARM:return "ARM";
    default:return std::nullopt;
  }
}
} // namespace
OsInfo collect_os(bool detail){
  OsInfo info;
  const auto build=win::reg_string(kVersionKey,"CurrentBuildNumber");
  const unsigned long build_value=build?std::strtoul(build->c_str(),nullptr,10):0;
  if(const auto name=win::reg_string(kVersionKey,"ProductName")) info.name=fix_windows_11_name(*name,build_value);
  if(const auto display=win::reg_string(kVersionKey,"DisplayVersion"))info.version=*display;
  else if(const auto release=win::reg_string(kVersionKey,"ReleaseId"))info.version=*release;
  if(build){
    std::string full=*build;
    if(const auto revision=win::reg_dword(kVersionKey,"UBR")){
      full+="."+std::to_string(*revision);info.build_revision=*revision;
    }
    info.build=full;
  }
  info.architecture=architecture_name();
  if(detail){
    info.uptime_seconds=GetTickCount64()/1000ULL;
    if(const auto installation=win::reg_dword(kVersionKey,"InstallDate")){
      const std::time_t timestamp=static_cast<std::time_t>(*installation);
      std::tm date{};
#if defined(_MSC_VER)
      if(localtime_s(&date,&timestamp)==0){
#else
      if(localtime_s(&date,&timestamp)==0){
#endif
        char buffer[32];if(std::strftime(buffer,sizeof buffer,"%Y-%m-%d",&date))info.install_date=buffer;
      }
    }
    DWORD count=0;
    GetComputerNameExA(ComputerNameDnsHostname,nullptr,&count);
    if(count){
      std::string name(count,'\0');
      if(GetComputerNameExA(ComputerNameDnsHostname,name.data(),&count)){
        name.resize(count);info.computer_name=name;
      }
    }
  }
  return info;
}
} // namespace hwspec
