#include "hwspec/collectors.hpp"
#include "registry.hpp"
#include <setupapi.h>
#include <initguid.h>
#include <batclass.h>
#include <vector>

namespace hwspec {
namespace {
void read_battery_details(BatteryInfo& info){
  HDEVINFO devices=SetupDiGetClassDevsW(&GUID_DEVINTERFACE_BATTERY,nullptr,nullptr,
                                        DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
  if(devices==INVALID_HANDLE_VALUE)return;
  for(DWORD n=0;n<64;++n){
    SP_DEVICE_INTERFACE_DATA interface_data{};interface_data.cbSize=sizeof interface_data;
    if(!SetupDiEnumDeviceInterfaces(devices,nullptr,&GUID_DEVINTERFACE_BATTERY,n,&interface_data))break;
    DWORD bytes=0;
    SetupDiGetDeviceInterfaceDetailW(devices,&interface_data,nullptr,0,&bytes,nullptr);
    if(bytes<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))continue;
    std::vector<BYTE> buffer(bytes);
    auto* path=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
    path->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    if(!SetupDiGetDeviceInterfaceDetailW(devices,&interface_data,path,bytes,nullptr,nullptr))continue;
    HANDLE handle=CreateFileW(path->DevicePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                              nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE)continue;
    DWORD wait=0,tag=0,returned=0;
    if(DeviceIoControl(handle,IOCTL_BATTERY_QUERY_TAG,&wait,sizeof wait,&tag,sizeof tag,&returned,nullptr) && tag){
      BATTERY_QUERY_INFORMATION request{};request.BatteryTag=tag;request.InformationLevel=BatteryInformation;
      BATTERY_INFORMATION result{};
      if(DeviceIoControl(handle,IOCTL_BATTERY_QUERY_INFORMATION,&request,sizeof request,
                         &result,sizeof result,&returned,nullptr)){
        // Relative capacities have no known Wh conversion. 0 and 0xFFFFFFFF
        // are unreported values, not valid absolute mWh measurements.
        if(!(result.Capabilities & BATTERY_CAPACITY_RELATIVE)){
          if(result.DesignedCapacity && result.DesignedCapacity!=0xFFFFFFFF)
            info.design_capacity_wh=static_cast<double>(result.DesignedCapacity)/1000.0;
          if(result.FullChargedCapacity && result.FullChargedCapacity!=0xFFFFFFFF)
            info.full_charge_capacity_wh=static_cast<double>(result.FullChargedCapacity)/1000.0;
        }
        if(result.CycleCount && result.CycleCount!=0xFFFFFFFF)
          info.cycle_count=result.CycleCount;
      }
      CloseHandle(handle);break;
    }
    CloseHandle(handle);
  }
  SetupDiDestroyDeviceInfoList(devices);
}
} // namespace
BatteryInfo collect_battery(bool detail){
  BatteryInfo info;
  SYSTEM_POWER_STATUS status{};
  if(!GetSystemPowerStatus(&status))return info;
  if(status.BatteryFlag==128){info.present=false;return info;}
  if(status.BatteryFlag==255)return info;
  info.present=true;
  if(status.BatteryLifePercent<=100)info.charge_percent=status.BatteryLifePercent;
  if(status.BatteryFlag&8)info.power_state="Charging";
  else if(status.ACLineStatus==1)info.power_state="Plugged in";
  else if(status.ACLineStatus==0)info.power_state="On battery";
  if(detail)read_battery_details(info);
  return info;
}
} // namespace hwspec
