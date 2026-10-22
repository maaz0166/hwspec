#pragma once

#include <string>
#include <vector>

namespace hwspec {

// One line in a section: a label and an already formatted value.
struct Field {
  std::string label;
  std::string value;
};

// A titled group of fields, for example "CPU" with its name, cores and clock.
struct SectionReport {
  std::string title;
  std::vector<Field> fields;
};

// Turns reports into text. Labels are aligned inside each section and sections are
// separated by a blank line.
std::string render_text(const std::vector<SectionReport>& reports);

}  // namespace hwspec
