#include "admin.hpp"
#include <algorithm>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <cstdio>
#endif

namespace hwspec::cli {
std::vector<std::string> elevated_arguments(const std::vector<std::string>& args) {
  std::vector<std::string> result;
  for (const auto& arg : args)
    if (arg != "--elevate" && arg != "--pause") result.push_back(arg);
  result.push_back("--pause");
  return result;
}

std::wstring quote_windows_argument(const std::wstring& value) {
  std::wstring result = L"\"";
  std::size_t slashes = 0;
  for (wchar_t c : value) {
    if (c == L'\\') { ++slashes; continue; }
    result.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
    result += c;
    slashes = 0;
  }
  result.append(slashes * 2, L'\\');
  return result + L'"';
}

#ifdef _WIN32
namespace {
bool is_elevated() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
  TOKEN_ELEVATION elevation{};
  DWORD bytes = 0;
  const bool ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof elevation, &bytes) != FALSE;
  CloseHandle(token);
  return ok && elevation.TokenIsElevated != 0;
}
bool relaunch(const std::vector<std::string>& args) {
  std::wstring executable(32768, L'\0');
  const DWORD size = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
  if (!size || size >= executable.size()) return false;
  executable.resize(size);
  std::wstring parameters;
  // All accepted CLI arguments are ASCII flags; no paths or positional values.
  for (const auto& arg : args) {
    if (!parameters.empty()) parameters += L' ';
    parameters += quote_windows_argument(std::wstring(arg.begin(), arg.end()));
  }
  SHELLEXECUTEINFOW info{};
  info.cbSize = sizeof info;
  info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
  info.lpVerb = L"runas";
  info.lpFile = executable.c_str();
  info.lpParameters = parameters.c_str();
  info.nShow = SW_SHOWNORMAL;
  return ShellExecuteExW(&info) != FALSE;
}
} // namespace
#endif

AdminRuntime native_admin_runtime(std::ostream& err) {
  AdminRuntime runtime;
#ifdef _WIN32
  runtime.elevated = is_elevated();
  runtime.interactive = _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
  runtime.relaunch = relaunch;
#endif
  runtime.prompt = [&err](const std::vector<std::string>& titles) {
    err << titles.size() << " section(s) need administrator rights: ";
    for (std::size_t i = 0; i < titles.size(); ++i) {
      if (i) err << ", ";
      err << titles[i];
    }
    err << ".\n  [1] Skip them and continue (default)\n"
        << "  [2] Reopen this command as administrator\n  [3] Cancel\nChoice [1]: " << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    return answer == "2" ? '2' : answer == "3" ? '3' : '1';
  };
  runtime.pause = [&err] {
    err << "Press Enter to close..." << std::flush;
    std::string ignored;
    std::getline(std::cin, ignored);
  };
  return runtime;
}
} // namespace hwspec::cli
