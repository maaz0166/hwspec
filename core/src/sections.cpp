#include "hwspec/sections.hpp"

#include "hwspec/collectors.hpp"
#include "hwspec/format.hpp"

namespace hwspec {

const std::vector<Section>& default_sections() {
  // None of the M1 sections needs administrator rights, so requires_admin is false everywhere.
  // Later sections (such as disk health) will set it to true; the prompt itself arrives in M3.
  static const std::vector<Section> sections = {
      {"cpu", "CPU", false, [] { return format_cpu(collect_cpu()); }},
      {"mem", "Memory", false, [] { return format_mem(collect_mem()); }},
      {"os", "OS", false, [] { return format_os(collect_os()); }},
  };
  return sections;
}

}  // namespace hwspec
