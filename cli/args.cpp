#include "args.hpp"

#include <algorithm>

namespace hwspec::cli {

namespace {

bool is_known(const std::string& flag, const std::vector<Section>& known) {
  return std::any_of(known.begin(), known.end(), [&](const Section& s) { return s.flag == flag; });
}

}  // namespace

Args parse_args(const std::vector<std::string>& args, const std::vector<Section>& known) {
  Args result;

  for (const std::string& arg : args) {
    if (arg == "--help" || arg == "-h") {
      result.help = true;
    } else if (arg == "--version" || arg == "-v") {
      result.version = true;
    } else if (arg == "--detail" || arg == "-d") {
      result.detail = true;
    } else if (arg == "--all") {
      result.all = true;
    } else if (arg.rfind("--", 0) == 0 && is_known(arg.substr(2), known)) {
      const std::string flag = arg.substr(2);
      if (std::find(result.sections.begin(), result.sections.end(), flag) == result.sections.end()) {
        result.sections.push_back(flag);  // asking twice has the same effect as asking once
      }
    } else if (!arg.empty() && arg[0] == '-') {
      result.error = "unknown option '" + arg + "'";
      return result;
    } else {
      result.error = "unexpected argument '" + arg + "'";
      return result;
    }
  }

  return result;
}

}  // namespace hwspec::cli
