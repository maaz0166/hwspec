#include "run.hpp"

#include <algorithm>
#include <ostream>

#include "args.hpp"
#include "hwspec/report.hpp"
#include "hwspec/version.hpp"

namespace hwspec::cli {

namespace {

void print_help(std::ostream& out, const std::vector<Section>& sections) {
  out << "hwspec " << hwspec::version() << " - PC hardware specs in one command\n"
      << "\n"
      << "Usage:\n"
      << "  hwspec [options]\n"
      << "\n"
      << "Sections (combine as many as you like; with none, every section is shown):\n";
  // Descriptions line up at the longest flag name ("all" counts too), plus two spaces.
  std::size_t width = std::string("all").size();
  for (const Section& section : sections) {
    width = std::max(width, section.flag.size());
  }
  for (const Section& section : sections) {
    out << "  --" << section.flag << std::string(width - section.flag.size() + 2, ' ') << "Show " << section.title
        << " details\n";
  }
  out << "  --all" << std::string(width - 3 + 2, ' ') << "Show every section\n"
      << "\n"
      << "Options:\n"
      << "  -d, --detail   More detail per section (not available yet)\n"
      << "  -h, --help     Show this help\n"
      << "  -v, --version  Show the version\n";
}

}  // namespace

int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err,
        const std::vector<Section>& sections) {
  const Args parsed = parse_args(args, sections);

  if (!parsed.error.empty()) {
    err << "hwspec: " << parsed.error << ". Try 'hwspec --help'.\n";
    return 2;
  }
  if (parsed.help) {
    print_help(out, sections);
    return 0;
  }
  if (parsed.version) {
    out << "hwspec " << hwspec::version() << "\n";
    return 0;
  }
  if (parsed.detail) {
    err << "hwspec: note: --detail is not available yet; showing the normal view.\n";
  }

  // No section flag (or --all) means every section. Output always follows the order of the
  // section list, whatever order the flags were typed in.
  const bool everything = parsed.all || parsed.sections.empty();

  std::vector<SectionReport> reports;
  for (const Section& section : sections) {
    const bool chosen =
        everything || std::find(parsed.sections.begin(), parsed.sections.end(), section.flag) != parsed.sections.end();
    if (chosen) {
      reports.push_back(section.collect());
    }
  }

  out << render_text(reports);
  return 0;
}

}  // namespace hwspec::cli
