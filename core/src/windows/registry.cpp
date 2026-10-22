#include "registry.hpp"
#include "wmi.hpp" // utf8 conversion; the registry may contain non-ASCII names
#include <cstring>
#include <vector>

namespace hwspec::win {
std::optional<std::string> reg_string(const char* subkey,const char* value){
  // Use the Unicode Win32 API and always return UTF-8 for valid JSON output.
  const std::wstring key(subkey,subkey+std::strlen(subkey));
  const std::wstring name(value,value+std::strlen(value));
  DWORD bytes=0;
  if(RegGetValueW(HKEY_LOCAL_MACHINE,key.c_str(),name.c_str(),RRF_RT_REG_SZ,
                  nullptr,nullptr,&bytes)!=ERROR_SUCCESS || !bytes || bytes>1024*1024)
    return std::nullopt;
  std::vector<wchar_t> buffer(bytes/sizeof(wchar_t)+1,L'\0');
  if(RegGetValueW(HKEY_LOCAL_MACHINE,key.c_str(),name.c_str(),RRF_RT_REG_SZ,
                  nullptr,buffer.data(),&bytes)!=ERROR_SUCCESS)return std::nullopt;
  const auto value_utf8=utf8(buffer.data());
  return !value_utf8.empty()?std::optional<std::string>(value_utf8):std::nullopt;
}
std::optional<DWORD> reg_dword(const char* subkey,const char* value){
  DWORD data=0,bytes=sizeof data;
  if(RegGetValueA(HKEY_LOCAL_MACHINE,subkey,value,RRF_RT_REG_DWORD,
                  nullptr,&data,&bytes)!=ERROR_SUCCESS)return std::nullopt;
  return data;
}
} // namespace hwspec::win
