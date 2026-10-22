#include "run.hpp"
#include <algorithm>
#include <ostream>
#include "args.hpp"
#include "hwspec/json.hpp"
#include "hwspec/report.hpp"
#include "hwspec/version.hpp"

namespace hwspec::cli {
namespace {
void print_help(std::ostream& out, const std::vector<Section>& sections) {
  out << "hwspec " << version() << " - Windows PC hardware specs\n\n"
      << "Usage: hwspec [sections] [-d] [--json]\n\n"
      << "Sections (combine flags; no section means --all):\n";
  std::size_t width = 3;
  for (const auto& s : sections) width = std::max(width,s.flag.size());
  for (const auto& s : sections)
    out << "  --" << s.flag << std::string(width-s.flag.size()+2,' ') << "Show " << s.title << "\n";
  out << "  --all" << std::string(width-3+2,' ') << "Show all sections\n\n"
      << "Options:\n"
      << "  -d, --detail   Add available hardware details\n"
      << "  --json         Machine-readable JSON (null for unknown data)\n"
      << "  --no-color     Disable color (text is currently uncolored)\n"
      << "  --skip-admin   Skip privileged data without prompting\n"
      << "  --elevate      Reopen as administrator (Windows UAC)\n"
      << "  --pause        Wait for Enter in an interactive text console\n"
      << "  -h, --help     Show this help\n"
      << "  -v, --version  Show the version\n";
}
} // namespace

int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err,
        const std::vector<Section>& sections) {
  return run(args, out, err, sections, native_admin_runtime(err));
}

int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err,
        const std::vector<Section>& sections, const AdminRuntime& runtime) {
  const Args parsed = parse_args(args, sections);
  if (!parsed.error.empty()) {
    err << "hwspec: " << parsed.error << ". Try 'hwspec --help'.\n";
    return 2;
  }
  if (parsed.help) { print_help(out, sections); return 0; }
  if (parsed.version) { out << "hwspec " << version() << "\n"; return 0; }

  const bool everything = parsed.all || parsed.sections.empty();
  const auto selected = [&](const Section& section) {
    return everything || std::find(parsed.sections.begin(), parsed.sections.end(), section.flag) != parsed.sections.end();
  };
  std::vector<std::string> admin_titles;
  for (const auto& section : sections)
    if (selected(section) && (section.requires_admin || (parsed.detail && section.detail_requires_admin)))
      admin_titles.push_back(section.admin_title.empty() ? section.title : section.admin_title);

  // --skip-admin is also useful in an already elevated shell.
  const bool allow_admin = runtime.elevated && !parsed.skip_admin;
  if (!runtime.elevated) {
    bool elevate = parsed.elevate;
    if (!elevate && !admin_titles.empty() && runtime.interactive && !parsed.json && !parsed.skip_admin && runtime.prompt) {
      const char choice = runtime.prompt(admin_titles);
      if (choice == '3') return 1;
      elevate = choice == '2';
    }
    if (elevate) {
      if (runtime.relaunch && runtime.relaunch(elevated_arguments(args))) return 0;
      err << "hwspec: elevation declined or unavailable; continuing without administrator data.\n";
    }
  }
  std::vector<SectionReport> reports;
  bool first = true;
  if (parsed.json) out << "{";
  for (const auto& section : sections) {
    if (!selected(section))
      continue;
    if (parsed.json) {
      if (!first) out << ",";
      first = false;
      out << json_quote(section.flag) << ":";
      if (section.requires_admin && !allow_admin) out << "{\"status\":\"requires_admin\"}";
      else if (section.collect_json_admin) out << section.collect_json_admin(parsed.detail, allow_admin);
      else if (section.collect_json) out << section.collect_json(parsed.detail);
      else { // Only used by M1-style plugin/fake sections.
        out << "{";
        const SectionReport fallback = parsed.detail && section.collect_detail ? section.collect_detail() : section.collect();
        for (std::size_t i=0;i<fallback.fields.size();++i) {
          if (i) out << ",";
          out << json_quote(fallback.fields[i].label) << ":" << json_quote(fallback.fields[i].value);
        }
        out << "}";
      }
    } else {
      if (section.requires_admin && !allow_admin)
        reports.push_back({section.title, {{"Status", "requires admin"}}});
      else if (parsed.detail && section.collect_detail_admin)
        reports.push_back(section.collect_detail_admin(allow_admin));
      else reports.push_back(parsed.detail && section.collect_detail ? section.collect_detail() : section.collect());
    }
  }
  if (parsed.json) out << "}\n";
  else out << render_text(reports);
  if (parsed.pause && runtime.interactive && !parsed.json && runtime.pause) {
    out.flush();
    runtime.pause();
  }
  return 0;
}
} // namespace hwspec::cli
