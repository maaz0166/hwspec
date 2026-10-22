#include "hwspec/collectors.hpp"
#include "registry.hpp"
#include "wmi.hpp"
#include <windows.h>
#include <winioctl.h>
#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace hwspec {
namespace {
std::optional<std::string> desc_string(const char* data,DWORD length,DWORD offset){
  if(!offset || offset>=length)return std::nullopt;
  const char* start=data+offset;
  const char* end=static_cast<const char*>(std::memchr(start,'\0',length-offset));
  if(!end || end==start)return std::nullopt;
  return std::string(start,end);
}
std::string bus_name(STORAGE_BUS_TYPE bus){
  switch(bus){
    case BusTypeScsi:return "SCSI";case BusTypeAtapi:return "ATAPI";
    case BusTypeAta:return "ATA";case BusType1394:return "FireWire";
    case BusTypeSsa:return "SSA";case BusTypeFibre:return "Fibre";
    case BusTypeUsb:return "USB";case BusTypeRAID:return "RAID";
    case BusTypeiScsi:return "iSCSI";case BusTypeSas:return "SAS";
    case BusTypeSata:return "SATA";case BusTypeSd:return "SD";
    case BusTypeMmc:return "MMC";case BusTypeVirtual:return "Virtual";
    case BusTypeFileBackedVirtual:return "File-backed virtual";
    case BusTypeSpaces:return "Storage Spaces";case BusTypeNvme:return "NVMe";
    default:return "Unknown";
  }
}
void physical_disk_details(const std::string& drive,DiskDrive& disk){
  // A volume maps to a physical device; dynamic/spanned volumes may map to
  // multiple disks, in which case we do NOT assign one model to the whole volume.
  const std::string volume="\\\\.\\"+drive.substr(0,2);
  HANDLE h=CreateFileA(volume.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,
                        nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(h==INVALID_HANDLE_VALUE)return;
  std::array<char,2048> extents{};DWORD returned=0;
  const bool mapped=DeviceIoControl(h,IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,nullptr,0,
                 extents.data(),static_cast<DWORD>(extents.size()),&returned,nullptr)!=FALSE;
  CloseHandle(h);
  if(!mapped || returned<sizeof(VOLUME_DISK_EXTENTS))return;
  const auto* mapping=reinterpret_cast<const VOLUME_DISK_EXTENTS*>(extents.data());
  if(mapping->NumberOfDiskExtents!=1)return;
  const std::string path="\\\\.\\PhysicalDrive"+std::to_string(mapping->Extents[0].DiskNumber);
  h=CreateFileA(path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,
                nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(h==INVALID_HANDLE_VALUE)return;
  STORAGE_PROPERTY_QUERY query{};query.PropertyId=StorageDeviceProperty;query.QueryType=PropertyStandardQuery;
  std::array<char,4096> data{};returned=0;
  if(DeviceIoControl(h,IOCTL_STORAGE_QUERY_PROPERTY,&query,sizeof query,data.data(),
      static_cast<DWORD>(data.size()),&returned,nullptr) && returned>=sizeof(STORAGE_DEVICE_DESCRIPTOR)){
    const auto* desc=reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(data.data());
    disk.model=desc_string(data.data(),returned,desc->ProductIdOffset);
    if(!disk.model)disk.model=desc_string(data.data(),returned,desc->VendorIdOffset);
    disk.firmware=desc_string(data.data(),returned,desc->ProductRevisionOffset);
    if(desc->BusType!=BusTypeUnknown)disk.bus=bus_name(desc->BusType);
  }
  query={};query.PropertyId=StorageDeviceSeekPenaltyProperty;query.QueryType=PropertyStandardQuery;
  DEVICE_SEEK_PENALTY_DESCRIPTOR penalty{};
  if(DeviceIoControl(h,IOCTL_STORAGE_QUERY_PROPERTY,&query,sizeof query,&penalty,sizeof penalty,&returned,nullptr)
     && returned>=sizeof penalty){disk.media_type=penalty.IncursSeekPenalty?"HDD":"SSD";}
  CloseHandle(h);
}
} // namespace
DiskInfo collect_disk(bool detail){
  DiskInfo info;
  const DWORD length=GetLogicalDriveStringsA(0,nullptr);
  if(!length || length>32768)return info;
  std::vector<char> drives(length+1,'\0');
  if(!GetLogicalDriveStringsA(length,drives.data()))return info;
  for(const char* path=drives.data();*path;path+=std::strlen(path)+1){
    const UINT type=GetDriveTypeA(path);
    if(type!=DRIVE_FIXED && type!=DRIVE_REMOVABLE && type!=DRIVE_REMOTE)continue;
    DiskDrive disk;disk.letter=std::string(path);
    ULARGE_INTEGER free_available{},total{},total_free{};
    if(GetDiskFreeSpaceExA(path,&free_available,&total,&total_free)){
      disk.size_bytes=static_cast<std::uint64_t>(total.QuadPart);
      disk.free_bytes=static_cast<std::uint64_t>(total_free.QuadPart);
    }
    if(detail){
      char filesystem[MAX_PATH+1]{};
      if(GetVolumeInformationA(path,nullptr,0,nullptr,nullptr,nullptr,filesystem,MAX_PATH))
        disk.file_system=filesystem;
      if(type!=DRIVE_REMOTE)physical_disk_details(path,disk);
    }
    info.drives.push_back(std::move(disk));
  }
  return info;
}

DiskInfo collect_disk_with_health(bool detail, bool allow_admin){
  DiskInfo info=collect_disk(detail);
  if(!detail || !allow_admin)return info;
  info.health_status=DiskHealthStatus::unknown;
  const auto rows=win::wmi_query(
      L"SELECT InstanceName, PredictFailure FROM MSStorageDriver_FailurePredictStatus WHERE Active = TRUE",
      {"InstanceName","PredictFailure"},L"ROOT\\WMI");
  for(const auto& row:rows){
    DiskHealthDevice device;
    const auto name=row.find("InstanceName");
    if(name!=row.end() && !name->second.empty())device.instance_name=name->second;
    const auto failure=row.find("PredictFailure");
    if(failure!=row.end()){
      if(failure->second=="True" || failure->second=="true" || failure->second=="-1" || failure->second=="1")
        device.failure_predicted=true;
      else if(failure->second=="False" || failure->second=="false" || failure->second=="0")
        device.failure_predicted=false;
    }
    if(device.failure_predicted.has_value())info.health_status=DiskHealthStatus::available;
    info.health_devices.push_back(std::move(device));
  }
  return info;
}
} // namespace hwspec
