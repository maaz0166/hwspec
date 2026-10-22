#pragma once

#include <string>
#include <vector>

#include "hwspec/sections.hpp"

namespace hwspec::cli {

// What the user asked for on the command line.
struct Args {
  bool help = false;
  bool version = false;
  bool detail = false;                // -d / --detail
  bool all = false;                   // --all
  std::vector<std::string> sections;  // section flags given, such as "cpu", without duplicates
  std::string error;                  // empty when parsing succeeded
};

// Reads the arguments (without the program name). Section flags are matched against `known`,
// so adding a section to the list automatically adds its --flag.
Args parse_args(const std::vector<std::string>& args, const std::vector<Section>& known);

}  // namespace hwspec::cli
