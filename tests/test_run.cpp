#include <sstream>

#include "check.hpp"
#include "hwspec/version.hpp"
#include "run.hpp"

using namespace hwspec;
using namespace hwspec::cli;

namespace {

// Fake sections with fixed output, so the test does not depend on the machine it runs on.
const std::vector<Section> kSections = {
    {"cpu", "CPU", false, [] { return SectionReport{"CPU", {{"Name", "Fake CPU"}}}; }},
    {"mem", "Memory", false, [] { return SectionReport{"Memory", {{"Total", "16.0 GB"}}}; }},
};

struct Result {
  int code;
  std::string out;
  std::string err;
};

Result run_with(const std::vector<std::string>& args) {
  std::ostringstream out;
  std::ostringstream err;
  const int code = run(args, out, err, kSections);
  return {code, out.str(), err.str()};
}

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

void test_no_arguments_shows_everything() {
  const Result r = run_with({});
  CHECK_EQ(r.code, 0);
  CHECK_EQ(r.out, std::string("CPU\n  Name  Fake CPU\n\nMemory\n  Total  16.0 GB\n"));
  CHECK(r.err.empty());
}

void test_single_section() {
  const Result r = run_with({"--mem"});
  CHECK_EQ(r.code, 0);
  CHECK_EQ(r.out, std::string("Memory\n  Total  16.0 GB\n"));
}

void test_output_order_follows_the_section_list() {
  const Result r = run_with({"--mem", "--cpu"});
  CHECK_EQ(r.out, std::string("CPU\n  Name  Fake CPU\n\nMemory\n  Total  16.0 GB\n"));
}

void test_all_flag() {
  const Result r = run_with({"--all"});
  CHECK(contains(r.out, "CPU"));
  CHECK(contains(r.out, "Memory"));
}

void test_version() {
  const Result r = run_with({"--version"});
  CHECK_EQ(r.code, 0);
  CHECK_EQ(r.out, std::string("hwspec ") + hwspec::version() + "\n");
}

void test_help_lists_sections() {
  const Result r = run_with({"--help"});
  CHECK_EQ(r.code, 0);
  CHECK(contains(r.out, "--cpu"));
  CHECK(contains(r.out, "--mem"));
  CHECK(contains(r.out, "--all"));
  CHECK(contains(r.out, "--detail"));
}

void test_unknown_option() {
  const Result r = run_with({"--bogus"});
  CHECK_EQ(r.code, 2);
  CHECK(r.out.empty());
  CHECK(contains(r.err, "unknown option '--bogus'"));
}

void test_detail_prints_a_note_and_still_works() {
  const Result r = run_with({"--cpu", "-d"});
  CHECK_EQ(r.code, 0);
  CHECK_EQ(r.out, std::string("CPU\n  Name  Fake CPU\n"));
  CHECK(contains(r.err, "not available yet"));
}

}  // namespace

int main() {
  test_no_arguments_shows_everything();
  test_single_section();
  test_output_order_follows_the_section_list();
  test_all_flag();
  test_version();
  test_help_lists_sections();
  test_unknown_option();
  test_detail_prints_a_note_and_still_works();
  return finish();
}
