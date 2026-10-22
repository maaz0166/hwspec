#define _WIN32_DCOM
#include <windows.h>
#include <wbemidl.h>
#include <oleauto.h>
#include "wmi.hpp"

namespace hwspec::win {
std::string utf8(const wchar_t* text){
  if(!text)return {};
  const int n=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);
  if(n<=1)return {};
  std::string result(static_cast<std::size_t>(n),'\0');
  if(!WideCharToMultiByte(CP_UTF8,0,text,-1,result.data(),n,nullptr,nullptr))return {};
  result.pop_back();
  return result;
}
std::vector<WmiRow> wmi_query(const wchar_t* query,const std::vector<std::string>& properties){
  std::vector<WmiRow> results;
  HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  const bool undo=SUCCEEDED(hr);
  if(FAILED(hr) && hr!=RPC_E_CHANGED_MODE)return results;
  // RPC_E_TOO_LATE is expected if another component already configured COM security.
  hr=CoInitializeSecurity(nullptr,-1,nullptr,nullptr,RPC_C_AUTHN_LEVEL_DEFAULT,
                          RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE,nullptr);
  if(FAILED(hr) && hr!=RPC_E_TOO_LATE){if(undo)CoUninitialize();return results;}
  IWbemLocator* locator=nullptr;IWbemServices* service=nullptr;IEnumWbemClassObject* enumerator=nullptr;
  hr=CoCreateInstance(CLSID_WbemLocator,nullptr,CLSCTX_INPROC_SERVER,IID_IWbemLocator,
                      reinterpret_cast<void**>(&locator));
  if(SUCCEEDED(hr)){
    BSTR ns=SysAllocString(L"ROOT\\CIMV2");
    hr=locator->ConnectServer(ns,nullptr,nullptr,nullptr,0,nullptr,nullptr,&service);
    SysFreeString(ns);
  }
  if(SUCCEEDED(hr))hr=CoSetProxyBlanket(service,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,nullptr,
                       RPC_C_AUTHN_LEVEL_CALL,RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE);
  if(SUCCEEDED(hr)){
    BSTR lang=SysAllocString(L"WQL"),q=SysAllocString(query);
    hr=service->ExecQuery(lang,q,WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,nullptr,&enumerator);
    SysFreeString(lang);SysFreeString(q);
  }
  if(SUCCEEDED(hr) && enumerator){
    for(unsigned count=0;count<128;++count){
      IWbemClassObject* row=nullptr;ULONG fetched=0;
      const HRESULT next=enumerator->Next(750,1,&row,&fetched);
      if(next!=WBEM_S_NO_ERROR || fetched==0 || !row)break;
      WmiRow data;
      for(const auto& key:properties){
        // WMI property names in this application are ASCII.
        std::wstring wide(key.begin(),key.end());
        VARIANT value;VariantInit(&value);
        if(SUCCEEDED(row->Get(wide.c_str(),0,&value,nullptr,nullptr)) && value.vt!=VT_NULL && value.vt!=VT_EMPTY){
          VARIANT as_text;VariantInit(&as_text);
          if(SUCCEEDED(VariantChangeType(&as_text,&value,0,VT_BSTR)) && as_text.vt==VT_BSTR && as_text.bstrVal)
            data[key]=utf8(as_text.bstrVal);
          VariantClear(&as_text);
        }
        VariantClear(&value);
      }
      results.push_back(std::move(data));row->Release();
    }
  }
  if(enumerator)enumerator->Release();
  if(service)service->Release();
  if(locator)locator->Release();
  if(undo)CoUninitialize();
  return results;
}
} // namespace hwspec::win
