// Windows only: checks that the real collectors return plausible values on this machine.
// Exact numbers differ on every PC, so we only test that they make sense.

#include "check.hpp"
#include "hwspec/collectors.hpp"

using namespace hwspec;

namespace {

void test_cpu() {
  const CpuInfo cpu = collect_cpu();
  CHECK(cpu.name.has_value());
  CHECK(cpu.logical_cores.has_value());
  if (cpu.logical_cores) {
    CHECK(*cpu.logical_cores >= 1);
  }
  if (cpu.physical_cores && cpu.logical_cores) {
    CHECK(*cpu.physical_cores >= 1);
    CHECK(*cpu.logical_cores >= *cpu.physical_cores);  // threads are never fewer than cores
  }
  if (cpu.base_clock_ghz) {
    CHECK(*cpu.base_clock_ghz > 0.1);
    CHECK(*cpu.base_clock_ghz < 10.0);
  }
}

void test_mem() {
  const MemInfo mem = collect_mem();
  CHECK(mem.total_bytes.has_value());
  CHECK(mem.available_bytes.has_value());
  if (mem.total_bytes && mem.available_bytes) {
    CHECK(*mem.total_bytes > 0);
    CHECK(*mem.available_bytes <= *mem.total_bytes);
  }
}

void test_os() {
  const OsInfo os = collect_os();
  CHECK(os.name.has_value());
  CHECK(os.build.has_value());
  CHECK(os.architecture.has_value());
}

}  // namespace

int main() {
  test_cpu();
  test_mem();
  test_os();
  return finish();
}
