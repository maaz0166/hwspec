# hwspec

A small Windows 10/11 (x64) CLI for machine specifications, implemented in C++17.
The name **hwspec** is final; older project-plan examples saying `sysinfo` are placeholders.

**Status: M3 source implemented (v0.3.0).** Native Windows MinGW build, unit tests,
JSON smoke checks and laptop battery collection have been exercised locally.
M2 still needs a second PC and manual UI comparisons; M3 still needs native UAC
and elevated hardware acceptance. See [M2 validation](docs/M2_VALIDATION.md) and
[M3 validation](docs/M3_VALIDATION.md). M4–M6 have not started.

## Commands

```powershell
hwspec                          # all seven sections; normal mode
hwspec --cpu --mem --os         # choose sections in any order
hwspec --gpu --disk --net
hwspec --battery
hwspec --battery -d            # capacity, health and cycles when available
hwspec --cpu -d                # clocks, caches, CPUID, virtualization
hwspec --all --detail          # detailed view for every section
hwspec --disk -d --skip-admin  # disk details without privileged data or prompts
hwspec --disk -d --elevate     # reopen with Windows UAC; child text console pauses
hwspec --all --json            # machine-readable JSON
hwspec --battery --json -d     # typed JSON, null for unsupported fields
hwspec --no-color              # accepted; M2 text is uncolored
hwspec --help
```

`--detail`/`-d` adds fields; **normal mode omits** them from text and JSON.
`--all` overrides individual section flags. `--json` emits one JSON object per
section, in the order CPU, memory, OS, GPU, disk, network, battery. Arrays contain
multiple adapters/drives/RAM modules, numbers remain numbers, and missing values
are `null` (including battery presence when not detectable). In text they are shown as `Unknown`.

## M2 data collection

| Section | Normal | Additional details (`-d`) |
| --- | --- | --- |
| CPU | Name, cores, base clock | Vendor, current and maximum reported clock, L1/L2/L3 caches, family/model/stepping, extensions, virtualization firmware state |
| Memory | Total, used, available | RAM modules (slot, capacity, type, speed, manufacturer), page-file allocated and used |
| OS | Name, version, build, architecture | Uptime, install date, UBR revision, computer name |
| GPU | First DXGI adapter, dedicated VRAM | Every adapter, shared memory, vendor/device IDs, WMI driver version, matched active display mode |
| Disk | Logical drives, total and free capacity | File system, physical model, bus, firmware, SSD/HDD if Windows reports seek penalty |
| Network | Adapters, IPv4/IPv6 addresses | Link speed, connection state, MAC, gateway, DNS |
| Battery | Charge and power state or no-battery message | Design/full capacity, computed health %, cycle count |

Collection uses native Windows APIs first; RAM module/page-file and GPU driver
metadata rely on read-only WMI. WMI and device firmware may omit properties.
A battery must report **absolute mWh** values for health to be calculated; a
cycle count of zero or capacity reported in relative units remains unknown.

## Administrator handling (M3)

Only detailed disk health needs administrator rights. Normal `--disk` never
prompts. In an interactive text console, `--disk -d` offers skip (default), reopen
as administrator, or cancel. JSON, redirected runs and `--skip-admin` skip
privileged data without prompting. `--elevate` explicitly opens a new elevated
console with the same flags and `--pause`; its output stays in that console.
Declining UAC continues the original command with privileged data skipped.
`--pause` waits for Enter only in an interactive text console.

Detailed JSON contains `disk.disk_health.status`: `requires_admin` when skipped,
`unknown` when the query yields no usable values, and `available` when active
WMI device instances report a prediction. `devices` contains `instance_name`
and `failure_predicted` (boolean or null). Devices are reported separately from
logical volumes to avoid guessing mappings. A false prediction means no failure
is predicted; it does not guarantee a healthy disk. NVMe/USB/RAID drivers may
not support the legacy WMI SMART provider. Text uses `Unknown` for missing data.
`--skip-admin` also skips SMART in an already elevated shell.

Maximum boost clock is read from CPUID leaf 0x16 where available. Some CPUs do
not report it. The fallback registry MHz value for base clock may itself be
approximate, so compare CPU clock values with System Information and CPU-Z.
GPU display resolution is left unknown when hybrid graphics cannot be mapped
unambiguously. On VMs, some devices/batteries may not exist; empty arrays or
unknown fields do not mean zero hardware.

## Build & tests

This project **runs on Windows x64**. CMake tests for formatting/CLI also build
on Linux with placeholder collectors; a Linux build does not report Linux data.

### MSVC / Visual Studio (recommended)

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
python tests/smoke_json.py .\build\Release\hwspec.exe
.\build\Release\hwspec.exe --all --detail
```

### MinGW with CMake

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python tests/smoke_json.py .\build\hwspec.exe
.\build\hwspec.exe --all --detail
```

The Windows runner in `.github/workflows/ci.yml` builds MSVC, runs the full
suite, parses `--all --json` and `--all -d --json`, and uploads a CI artifact.
This does **not** constitute validation on two physical PCs; run the test matrix
in [`docs/M2_VALIDATION.md`](docs/M2_VALIDATION.md) to finish the milestone's
hardware acceptance check.

## Repository layout

- `core/include/hwspec/`: plain collector types, interfaces, formatter and JSON API
- `core/src/windows/`: Windows Win32/DXGI/WMI/device IOCTL collectors
- `core/src/stub/`: portable placeholder collectors for Linux test builds
- `core/src/format.cpp`, `json.cpp`: hardware-independent output functions
- `cli/`: arguments and reusable `run()` function (exe + future Python binding)
- `tests/`: CLI, admin decision table, format, JSON, hardware checks and smoke script
- `python/`: the original PyPI name-reservation placeholder; M4 binding not yet started

All section collectors return plain data without printing. Windows code stays
behind the core's public interfaces. The original M1 collector callbacks remain
compatible with existing tests/consumers.
