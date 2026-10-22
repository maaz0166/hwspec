#include "hwspec/collectors.hpp"
#include "registry.hpp"
#include "wmi.hpp"
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace hwspec {
namespace {
std::string pci_code(const char* label,unsigned value){
  char buffer[32];std::snprintf(buffer,sizeof buffer,"%s_%04X",label,value);return buffer;
}
std::string lowered(std::string s){
  std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;
}
bool matches_hardware(const std::string& id,const GpuAdapter& gpu){
  if(!gpu.vendor_id || !gpu.device_id)return false;
  return id.find(lowered(pci_code("VEN",*gpu.vendor_id)))!=std::string::npos &&
         id.find(lowered(pci_code("DEV",*gpu.device_id)))!=std::string::npos;
}
std::string from_wide(const wchar_t* text){return win::utf8(text);}
} // namespace
GpuInfo collect_gpu(bool detail){
  GpuInfo info;
  IDXGIFactory1* factory=nullptr;
  if(SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),reinterpret_cast<void**>(&factory))) && factory){
    for(UINT index=0;index<64;++index){
      IDXGIAdapter1* adapter=nullptr;
      if(factory->EnumAdapters1(index,&adapter)!=S_OK || !adapter)break;
      DXGI_ADAPTER_DESC1 desc{};
      if(SUCCEEDED(adapter->GetDesc1(&desc))){
        GpuAdapter gpu;
        gpu.name=from_wide(desc.Description);
        gpu.dedicated_bytes=static_cast<std::uint64_t>(desc.DedicatedVideoMemory);
        gpu.shared_bytes=static_cast<std::uint64_t>(desc.SharedSystemMemory);
        gpu.vendor_id=desc.VendorId;
        gpu.device_id=desc.DeviceId;
        info.adapters.push_back(std::move(gpu));
      }
      adapter->Release();
    }
    factory->Release();
  }
  if(!detail)return info;
  // WMI is only used in detail mode: Win32_VideoController exposes driver versions.
  const auto drivers=win::wmi_query(L"SELECT Name,DriverVersion,PNPDeviceID FROM Win32_VideoController",{"Name","DriverVersion","PNPDeviceID"});
  for(auto& gpu:info.adapters){
    for(const auto& row:drivers){
      const auto name=row.find("Name"),version=row.find("DriverVersion");
      const auto pnp=row.find("PNPDeviceID");
      const bool matched_name=name!=row.end() && gpu.name && lowered(name->second)==lowered(*gpu.name);
      const bool matched_pci=pnp!=row.end() && matches_hardware(lowered(pnp->second),gpu);
      if(version!=row.end() && (matched_name || matched_pci)){
        gpu.driver_version=version->second;break;
      }
    }
  }
  // Display modes are associated with an active desktop display; Windows does
  // not always expose a one-to-one display-to-GPU mapping (Optimus/hybrid GPUs).
  // Only assign a mode when a display name matches the adapter description.
  DISPLAY_DEVICEW display{};display.cb=sizeof display;
  for(DWORD index=0;EnumDisplayDevicesW(nullptr,index,&display,0);++index){
    if(display.StateFlags & DISPLAY_DEVICE_ACTIVE){
      DEVMODEW mode{};mode.dmSize=sizeof mode;
      if(EnumDisplaySettingsW(display.DeviceName,ENUM_CURRENT_SETTINGS,&mode)){
        for(auto& gpu:info.adapters){
          if((gpu.name && lowered(from_wide(display.DeviceString))==lowered(*gpu.name)) ||
              matches_hardware(lowered(from_wide(display.DeviceID)),gpu)){
            gpu.resolution=std::to_string(mode.dmPelsWidth)+" x "+std::to_string(mode.dmPelsHeight);
            if(mode.dmDisplayFrequency>1 && mode.dmDisplayFrequency<1000)gpu.refresh_hz=mode.dmDisplayFrequency;
          }
        }
      }
    }
    display={};display.cb=sizeof display;
  }
  return info;
}
} // namespace hwspec
