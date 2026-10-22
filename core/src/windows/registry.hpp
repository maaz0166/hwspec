#pragma once

// Small helpers for reading values from HKEY_LOCAL_MACHINE. Internal to the Windows collectors.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <optional>
#include <string>

namespace hwspec::win {

// Returns the text value, or nothing if the key or value does not exist.
std::optional<std::string> reg_string(const char* subkey, const char* value);

// Returns the number value, or nothing if the key or value does not exist.
std::optional<DWORD> reg_dword(const char* subkey, const char* value);

}  // namespace hwspec::win
