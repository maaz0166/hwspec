#include "registry.hpp"

#include <cstring>

namespace hwspec::win {

std::optional<std::string> reg_string(const char* subkey, const char* value) {
  // First call: ask how many bytes the value needs. Second call: read it.
  DWORD size = 0;
  if (RegGetValueA(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS ||
      size == 0) {
    return std::nullopt;
  }
  std::string text(size, '\0');
  if (RegGetValueA(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_SZ, nullptr, text.data(), &size) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  text.resize(std::strlen(text.c_str()));  // drop the terminating zero (and anything after it)
  return text;
}

std::optional<DWORD> reg_dword(const char* subkey, const char* value) {
  DWORD data = 0;
  DWORD size = sizeof(data);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_DWORD, nullptr, &data, &size) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  return data;
}

}  // namespace hwspec::win
