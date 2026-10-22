#pragma once
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace hwspec::cli {
// Injectable process/terminal operations keep the admin decision table testable.
struct AdminRuntime {
  bool elevated = false;
  bool interactive = false;
  std::function<char(const std::vector<std::string>&)> prompt;
  std::function<bool(const std::vector<std::string>&)> relaunch;
  std::function<void()> pause;
};
AdminRuntime native_admin_runtime(std::ostream& err);
struct RelaunchCommand {
  std::wstring executable;
  std::vector<std::wstring> prefix;
};
AdminRuntime native_admin_runtime(std::ostream& err, const RelaunchCommand& command);
std::wstring relaunch_parameters(const std::vector<std::string>& args,
                                 const std::vector<std::wstring>& prefix = {});
std::vector<std::string> elevated_arguments(const std::vector<std::string>& args);
std::wstring quote_windows_argument(const std::wstring& value);
} // namespace hwspec::cli
