#pragma once
#include <string>
#include <vector>
#include "hwspec/sections.hpp"

namespace hwspec::cli {
struct Args {
  bool help=false, version=false, detail=false, all=false, json=false, no_color=false;
  bool skip_admin=false, elevate=false, pause=false;
  std::vector<std::string> sections;
  std::string error;
};
Args parse_args(const std::vector<std::string>& args, const std::vector<Section>& known);
} // namespace hwspec::cli
