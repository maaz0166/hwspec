#include "args.hpp"
#include "check.hpp"

using namespace hwspec;
using namespace hwspec::cli;

namespace {

const std::vector<Section> kSections = {
    {"cpu", "CPU", false, [] { return SectionReport{}; }, {}, {}},
    {"mem", "Memory", false, [] { return SectionReport{}; }, {}, {}},
};

void test_empty() {
  const Args args = parse_args({}, kSections);
  CHECK(args.error.empty());
  CHECK(args.sections.empty());
  CHECK(!args.all);
  CHECK(!args.help);
  CHECK(!args.version);
  CHECK(!args.detail);
}

void test_section_flags() {
  const Args args = parse_args({"--mem", "--cpu"}, kSections);
  CHECK(args.error.empty());
  CHECK_EQ(args.sections.size(), static_cast<std::size_t>(2));
  CHECK_EQ(args.sections[0], std::string("mem"));  // keeps the order typed
  CHECK_EQ(args.sections[1], std::string("cpu"));
}

void test_duplicates_are_ignored() {
  const Args args = parse_args({"--cpu", "--cpu"}, kSections);
  CHECK_EQ(args.sections.size(), static_cast<std::size_t>(1));
}

void test_other_flags() {
  CHECK(parse_args({"--all"}, kSections).all);
  CHECK(parse_args({"--help"}, kSections).help);
  CHECK(parse_args({"-h"}, kSections).help);
  CHECK(parse_args({"--version"}, kSections).version);
  CHECK(parse_args({"-v"}, kSections).version);
  CHECK(parse_args({"--detail"}, kSections).detail);
  CHECK(parse_args({"-d"}, kSections).detail);
  CHECK(parse_args({"--json"}, kSections).json);
  CHECK(parse_args({"--no-color"}, kSections).no_color);
}

void test_detail_with_section() {
  const Args args = parse_args({"--cpu", "-d"}, kSections);
  CHECK(args.error.empty());
  CHECK(args.detail);
  CHECK_EQ(args.sections.size(), static_cast<std::size_t>(1));
}

void test_errors() {
  CHECK_EQ(parse_args({"--gpu"}, kSections).error, std::string("unknown option '--gpu'"));
  CHECK_EQ(parse_args({"-x"}, kSections).error, std::string("unknown option '-x'"));
  CHECK_EQ(parse_args({"--"}, kSections).error, std::string("unknown option '--'"));
  CHECK_EQ(parse_args({"cpu"}, kSections).error, std::string("unexpected argument 'cpu'"));
}

}  // namespace

int main() {
  test_empty();
  test_section_flags();
  test_duplicates_are_ignored();
  test_other_flags();
  test_detail_with_section();
  test_errors();
  return finish();
}
