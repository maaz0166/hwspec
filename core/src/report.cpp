#include "hwspec/report.hpp"

#include <algorithm>

namespace hwspec {

std::string render_text(const std::vector<SectionReport>& reports) {
  std::string out;
  for (std::size_t i = 0; i < reports.size(); ++i) {
    const SectionReport& report = reports[i];
    if (i > 0) {
      out += "\n";  // blank line between sections
    }
    out += report.title + "\n";

    std::size_t width = 0;
    for (const Field& field : report.fields) {
      width = std::max(width, field.label.size());
    }
    for (const Field& field : report.fields) {
      out += "  " + field.label + std::string(width - field.label.size() + 2, ' ') + field.value + "\n";
    }
  }
  return out;
}

}  // namespace hwspec
