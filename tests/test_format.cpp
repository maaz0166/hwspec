#include "check.hpp"
#include "hwspec/format.hpp"
#include "hwspec/report.hpp"

using namespace hwspec;

namespace {

void test_format_bytes() {
  CHECK_EQ(format_bytes(0), std::string("0 B"));
  CHECK_EQ(format_bytes(512), std::string("512 B"));
  CHECK_EQ(format_bytes(1024), std::string("1.0 KB"));
  CHECK_EQ(format_bytes(1536), std::string("1.5 KB"));
  CHECK_EQ(format_bytes(16ULL * 1024 * 1024 * 1024), std::string("16.0 GB"));
  CHECK_EQ(format_bytes(17000000000ULL), std::string("15.8 GB"));
}

void test_format_ghz() {
  CHECK_EQ(format_ghz(3.4), std::string("3.40 GHz"));
  CHECK_EQ(format_ghz(2.5), std::string("2.50 GHz"));
}

void test_format_cpu_full() {
  CpuInfo info;
  info.name = "Test CPU";
  info.physical_cores = 4;
  info.logical_cores = 8;
  info.base_clock_ghz = 3.4;

  const SectionReport report = format_cpu(info);
  CHECK_EQ(report.title, std::string("CPU"));
  CHECK_EQ(report.fields.size(), static_cast<std::size_t>(3));
  CHECK_EQ(report.fields[0].value, std::string("Test CPU"));
  CHECK_EQ(report.fields[1].value, std::string("4 physical, 8 logical"));
  CHECK_EQ(report.fields[2].value, std::string("3.40 GHz"));
}

void test_format_cpu_missing_values() {
  const SectionReport empty = format_cpu(CpuInfo{});
  CHECK_EQ(empty.fields[0].value, std::string("Unknown"));
  CHECK_EQ(empty.fields[1].value, std::string("Unknown"));
  CHECK_EQ(empty.fields[2].value, std::string("Unknown"));

  CpuInfo only_logical;
  only_logical.logical_cores = 8;
  CHECK_EQ(format_cpu(only_logical).fields[1].value, std::string("8 logical"));
}

void test_format_mem() {
  MemInfo info;
  info.total_bytes = 16ULL * 1024 * 1024 * 1024;
  info.available_bytes = 8ULL * 1024 * 1024 * 1024;

  const SectionReport report = format_mem(info);
  CHECK_EQ(report.title, std::string("Memory"));
  CHECK_EQ(report.fields[0].value, std::string("16.0 GB"));
  CHECK_EQ(report.fields[1].value, std::string("8.0 GB (50%)"));
  CHECK_EQ(report.fields[2].value, std::string("8.0 GB"));
}

void test_format_mem_edge_cases() {
  // Available can never exceed total: used is clamped to zero instead of wrapping around.
  MemInfo odd;
  odd.total_bytes = 1024;
  odd.available_bytes = 4096;
  CHECK_EQ(format_mem(odd).fields[1].value, std::string("0 B (0%)"));

  // Nothing known at all.
  CHECK_EQ(format_mem(MemInfo{}).fields[0].value, std::string("Unknown"));

  // Total known but available missing.
  MemInfo no_available;
  no_available.total_bytes = 1024;
  CHECK_EQ(format_mem(no_available).fields[1].value, std::string("Unknown"));
}

void test_format_os() {
  OsInfo info;
  info.name = "Windows 11 Pro";
  info.build = "22631.3007";

  const SectionReport report = format_os(info);
  CHECK_EQ(report.title, std::string("OS"));
  CHECK_EQ(report.fields[0].value, std::string("Windows 11 Pro"));
  CHECK_EQ(report.fields[1].value, std::string("Unknown"));  // version not given
  CHECK_EQ(report.fields[2].value, std::string("22631.3007"));
}

void test_render_text() {
  SectionReport cpu{"CPU", {{"Name", "Test CPU"}, {"Cores", "4 physical, 8 logical"}}};
  SectionReport os{"OS", {{"Name", "Windows"}}};

  CHECK_EQ(render_text({cpu}), std::string("CPU\n  Name   Test CPU\n  Cores  4 physical, 8 logical\n"));
  CHECK_EQ(render_text({cpu, os}),
           std::string("CPU\n  Name   Test CPU\n  Cores  4 physical, 8 logical\n\nOS\n  Name  Windows\n"));
  CHECK_EQ(render_text({}), std::string(""));
}

}  // namespace

int main() {
  test_format_bytes();
  test_format_ghz();
  test_format_cpu_full();
  test_format_cpu_missing_values();
  test_format_mem();
  test_format_mem_edge_cases();
  test_format_os();
  test_render_text();
  return finish();
}
