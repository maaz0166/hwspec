#pragma once
#include <map>
#include <string>
#include <vector>

namespace hwspec::win {
// Best-effort, read-only local WMI. An empty result never indicates zero hardware.
using WmiRow=std::map<std::string,std::string>;
std::vector<WmiRow> wmi_query(const wchar_t* query,const std::vector<std::string>& properties,
                            const wchar_t* name_space = L"ROOT\\CIMV2");
std::string utf8(const wchar_t* input);
} // namespace hwspec::win
