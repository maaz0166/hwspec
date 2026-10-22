#include "hwspec/sections.hpp"
#include "hwspec/collectors.hpp"
#include "hwspec/format.hpp"
#include "hwspec/json.hpp"

namespace hwspec {
const std::vector<Section>& default_sections() {
  // Disk health is explicitly deferred to M3 (admin handling). M2 uses only
  // standard-user APIs; the health field is marked not_implemented.
  static const std::vector<Section> sections = {
    {"cpu", "CPU", false, []{return format_cpu(collect_cpu(false));},
      []{return format_cpu(collect_cpu(true),true);}, [] (bool d){return json_cpu(collect_cpu(d),d);}},
    {"mem", "Memory", false, []{return format_mem(collect_mem(false));},
      []{return format_mem(collect_mem(true),true);}, [] (bool d){return json_mem(collect_mem(d),d);}},
    {"os", "OS", false, []{return format_os(collect_os(false));},
      []{return format_os(collect_os(true),true);}, [] (bool d){return json_os(collect_os(d),d);}},
    {"gpu", "GPU", false, []{return format_gpu(collect_gpu(false));},
      []{return format_gpu(collect_gpu(true),true);}, [] (bool d){return json_gpu(collect_gpu(d),d);}},
    {"disk", "Disk", false, []{return format_disk(collect_disk(false));},
      []{return format_disk(collect_disk(true),true);}, [] (bool d){return json_disk(collect_disk(d),d);}},
    {"net", "Network", false, []{return format_net(collect_net(false));},
      []{return format_net(collect_net(true),true);}, [] (bool d){return json_net(collect_net(d),d);}},
    {"battery", "Battery", false, []{return format_battery(collect_battery(false));},
      []{return format_battery(collect_battery(true),true);}, [] (bool d){return json_battery(collect_battery(d),d);}},
  };
  return sections;
}
} // namespace hwspec
