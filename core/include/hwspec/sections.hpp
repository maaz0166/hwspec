#pragma once

#include <functional>
#include <string>
#include <vector>

#include "hwspec/report.hpp"

namespace hwspec {

// One selectable part of the output (--cpu, --mem, ...).
struct Section {
  std::string flag;            // the word after "--", for example "cpu"
  std::string title;           // heading in the output, for example "CPU"
  bool requires_admin;         // true when reading it needs administrator rights (used from M3)
  std::function<SectionReport()> collect;  // reads the machine and returns a formatted report
};

// All sections, in the order they are printed.
const std::vector<Section>& default_sections();

}  // namespace hwspec
