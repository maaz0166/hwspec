#include "hwspec/collectors.hpp"
#include <winsock2.h>
#include "registry.hpp"
#include "wmi.hpp"
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

namespace hwspec {
namespace {
std::string ip_text(SOCKET_ADDRESS address){
  if(!address.lpSockaddr)return {};
  char result[NI_MAXHOST]{};
  if(getnameinfo(address.lpSockaddr,address.iSockaddrLength,result,sizeof result,
                 nullptr,0,NI_NUMERICHOST)==0)return result;
  return {};
}
std::string mac_text(const BYTE* a,ULONG len){
  std::string result;char byte[4];
  for(ULONG i=0;i<len;++i){if(i)result+=':';
    std::snprintf(byte,sizeof byte,"%02X",a[i]);result+=byte;}
  return result;
}
} // namespace
NetInfo collect_net(bool detail){
  NetInfo info;
  ULONG bytes=15000;
  std::vector<unsigned char> buffer(bytes);
  DWORD status=GetAdaptersAddresses(AF_UNSPEC,GAA_FLAG_INCLUDE_GATEWAYS,nullptr,
                 reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&bytes);
  if(status==ERROR_BUFFER_OVERFLOW){buffer.resize(bytes);status=GetAdaptersAddresses(AF_UNSPEC,GAA_FLAG_INCLUDE_GATEWAYS,
                                nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&bytes);}
  if(status!=NO_ERROR)return info;
  WSADATA winsock{};
  if(WSAStartup(MAKEWORD(2,2),&winsock)!=0)return info;
  for(auto* row=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());row;row=row->Next){
    if(row->IfType==IF_TYPE_SOFTWARE_LOOPBACK)continue;
    NetAdapter adapter;
    const std::string name=win::utf8(row->FriendlyName);
    if(!name.empty())adapter.name=name;
    for(auto* addr=row->FirstUnicastAddress;addr;addr=addr->Next){
      const auto text=ip_text(addr->Address);if(!text.empty())adapter.addresses.push_back(text);
    }
    if(detail){
      adapter.status=row->OperStatus==IfOperStatusUp?"Connected":"Disconnected";
      if(row->TransmitLinkSpeed && row->TransmitLinkSpeed!=std::numeric_limits<ULONG64>::max())
        adapter.link_speed_bps=row->TransmitLinkSpeed;
      if(row->PhysicalAddressLength)adapter.mac=mac_text(row->PhysicalAddress,row->PhysicalAddressLength);
      for(auto* gw=row->FirstGatewayAddress;gw;gw=gw->Next){
        const auto text=ip_text(gw->Address);if(!text.empty() && !adapter.gateway)adapter.gateway=text;
      }
      for(auto* dns=row->FirstDnsServerAddress;dns;dns=dns->Next){
        const auto text=ip_text(dns->Address);if(!text.empty())adapter.dns_servers.push_back(text);
      }
    }
    info.adapters.push_back(std::move(adapter));
  }
  WSACleanup();
  return info;
}
} // namespace hwspec
