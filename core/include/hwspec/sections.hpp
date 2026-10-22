#pragma once
#include <functional>
#include <string>
#include <vector>
#include "hwspec/report.hpp"

namespace hwspec {
// Preserve the M1 collector so existing consumers/tests can construct Section.
// Detail and JSON callbacks are optional, enabling machine-independent fakes.
struct Section {
  std::string flag;
  std::string title;
  bool requires_admin;
  std::function<SectionReport()> collect;
  std::function<SectionReport()> collect_detail;
  std::function<std::string(bool)> collect_json;
  // Detail-only privileged data must not block the standard-user section.
  bool detail_requires_admin = false;
  std::string admin_title = {};
  std::function<SectionReport(bool)> collect_detail_admin = {};
  std::function<std::string(bool, bool)> collect_json_admin = {};
};
const std::vector<Section>& default_sections();
}  // namespace hwspec
