# M2 acceptance checklist (Windows)

The M2 source has portable unit tests and has now been built and exercised
on one Windows laptop. M2 acceptance in the project plan
requires testing on at least two different PCs, including a laptop reporting
battery capacity/health. GitHub Actions Windows runners are virtual machines;
they cannot substitute for those two physical machines.

## How to verify

1. On each Windows x64 PC, build with the MSVC commands in the root README.
2. Run `ctest --test-dir build -C Release --output-on-failure`.
3. Run `python tests/smoke_json.py .\build\Release\hwspec.exe` (MSVC).
4. Run `hwspec --all` and compare CPU, memory, OS to Task Manager + `msinfo32`.
5. Run `hwspec --all -d` and compare RAM modules, CPU/cache, GPU, disk, network
   with Task Manager / Device Manager / `msinfo32` / CPU-Z as applicable.
6. Run `hwspec --battery -d` on a **laptop**. Verify nonzero absolute design
   and full-charge Wh values, with health = 100 * full / design. Some firmware
   will not expose these; this case must be recorded rather than guessed.
7. Run `hwspec --cpu --gpu --json -d` and parse it (`ConvertFrom-Json` in
   PowerShell). Repeat `hwspec --all --json` with output redirected to a file.
8. Compare normal vs `-d` views for every section, test combined flags in
   different orders and test `--help`, `--version`, invalid arguments.

## Test log (fill in on each machine)

| Check | PC 1 | PC 2 |
| --- | --- | --- |
| Windows version, x64 | Windows 11 Pro x64, 26100.6584 | Not yet verified |
| MSVC build and CTest | MSVC + MinGW Release builds and CTest pass | Not yet verified |
| JSON smoke script | Passed | Not yet verified |
| Normal mode matches system UI | CIM comparisons pass; UI pending | Not yet verified |
| CPU detailed fields / Unknown cases | Runs; manual comparison pending | Not yet verified |
| GPU / disk / network detailed fields | Runs; manual comparison pending | Not yet verified |
| Battery capacity & health (laptop) | Matches Windows battery report | Not yet verified |
| JSON redirected / no extra text | Passed | Not yet verified |

## Local audit: 2026-10-22

PC 1: Dell Latitude 5500, Windows 11 Pro x64, build 26100.6584,
Intel Core i5-8365U (4 physical / 8 logical cores), 8 GB installed RAM.

- Native MinGW-w64 GCC 15.2 / Ninja configure and build passed. Initial build
  found missing Windows power header prerequisites, a missing SDK structure
  declaration, and battery IOCTL/GUID header compatibility issues; these are fixed.
- All six pre-M3 CTest executables passed. After M3, all seven tests passed.
- Normal and detailed all-section JSON smoke checks passed; detailed text and
  piped output ran successfully. Every section is exercised by those commands.
- CPU name and core counts, Windows-visible physical memory (8395116544 bytes),
  and OS build including registry UBR matched Windows CIM/registry data.
  Comparing only CIM BuildNumber to the full build string initially differed
  because CIM omits the revision; comparing build plus UBR matches.
- Battery IOCTL output: design 60.800 Wh, full charge 58.854 Wh, health 96.8%.
  Windows `powercfg /batteryreport /xml` independently reports 60800 and
  58854 mWh. BatteryCycleCount is unknown. The WMI BatteryStaticData query
  failed on this machine, while the direct battery IOCTL succeeded.
- GPU, volume model/bus/firmware, memory modules and network details returned
  values. Hardware testing found UINT64_MAX link speeds on disconnected
  interfaces; the collector now treats that sentinel as Unknown / JSON null.
- CPU maximum boost clock is Unknown. Registry base clock (~1.896 GHz here)
  is an approximate fallback and still requires manual UI comparison.

The table above includes partial acceptance results: UI comparisons,
a second physical PC, and Intel/AMD coverage are still pending. CIM comparisons
do not claim Task Manager/System Information UI validation.

### MSVC follow-up: 2026-10-22

Located Visual Studio 2022 on the D: drive using `vswhere` (MSVC 19.44.35217.0,
Windows SDK 10.0.26100.0). The x64 Release build exposed a Windows `max` macro
collision in the network speed check; parenthesizing the standard-library
function name fixes it. The first CTest run then exposed dangling references
in `CHECK_EQ` when comparing fields indexed from temporary reports. The helper
now copies its operands; the existing formatter edge-case assertions pass.

```powershell
cmake -S . -B build/msvc -G "Visual Studio 17 2022" -A x64
cmake --build build/msvc --config Release --parallel 4
ctest --test-dir build/msvc -C Release --output-on-failure
python tests/smoke_json.py ./build/msvc/Release/hwspec.exe
./build/msvc/Release/hwspec.exe --all -d --skip-admin
```

The MSVC Release build, all seven CTest executables, JSON smoke checks, and
all-section detailed text run passed on the same laptop. This does not count
as a second-PC check or manual comparison with Task Manager/System Information.

### Repeatable acceptance runner

[verify_windows.ps1](../tests/verify_windows.ps1) now captures all seven sections
in normal/detailed text and JSON, plus default/all views, flag handling, admin
policy and selected CIM comparisons. On this laptop it passed 107 standard-user
checks and 108 elevated checks. The standard-user validation kit was also run
successfully with Windows PowerShell 5.1. These repeats remain PC 1 results;
they do not satisfy PC 2 or the manual UI comparison requirement.

See [Windows acceptance](./WINDOWS_ACCEPTANCE.md) for commands to run on a second PC
without Python, CMake or a compiler.

### System Information export comparison: 2026-10-22

Generated a report using Windows `msinfo32 /report` and compared it with the
native output. This supplies System Information data; Task Manager visual
comparison and the second PC remain outstanding.

- CPU name, 4 physical/8 logical cores, and 1896 MHz reported clock match.
- Windows build 26100 and visible memory match (7.82 GB in System Information,
  7.8 GB at hwspec's lower display precision). Installed module capacity is 8 GB.
- C: and D: volume capacities match exactly at 181152235520 and 73712177152
  bytes, with NTFS and physical disk model matching the export.
- Intel UHD Graphics 620, driver 31.0.101.2137 and 1920 x 1080 resolution match.
- **Source discrepancies remain:** DXGI dedicated video memory is 134217728
  bytes (128 MiB), while System Information/WMI AdapterRAM is 1073741824 bytes
  (1 GiB). EnumDisplaySettings reports 60 Hz; the export/WMI reports 59 Hz.
  Keep the existing native API values and source distinctions; neither matching
  those different properties nor an exact refresh rate is claimed. Microsoft
  documents [DXGI memory values](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter-getdesc)
  and the [dedicated/shared memory distinction](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc).
- **Virtualization correctness fix:** the export reports a hypervisor and VBS
  running, while the firmware availability API returned false. That API describes
  firmware enablement *and* operating-system availability, so false under an
  active hypervisor does not reliably prove firmware is disabled. The collector
  now emits Unknown/JSON null in that case. It preserves true for a positive
  availability result, and false when no hypervisor is present. Regression tests
  cover all four combinations and both output formats. This is a conservative
  interpretation of the [API's documented meaning](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent),
  using Microsoft's [CPUID hypervisor discovery](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery).

The original elevation-test console kept its executable open. The corrected
MSVC executable was linked into a separate runtime directory:

```powershell
cmake -S . -B build/msvc -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=C:/Users/DELL/Desktop/Projects/hwspec-git/build/msvc/verified
cmake --build build/msvc --config Release --parallel 4
ctest --test-dir build/msvc -C Release --output-on-failure
python tests/smoke_json.py ./build/msvc/verified/hwspec.exe
```

Both corrected MSVC and MinGW builds pass all seven tests and JSON smoke checks.
The updated acceptance runner adds a hypervisor/virtualization consistency check
on hosts with a hypervisor (108 standard-user and 109 elevated checks on this
laptop, all passing). The final kit includes this corrected executable.

## Intentional scope boundaries and limitations

- Admin elevation, prompts, `--elevate`, `--skip-admin`, `--pause` and disk SMART
  were deferred during M2 and are now implemented in M3; see
  [M3 validation](./M3_VALIDATION.md) for behavior and remaining acceptance checks.
- `--report`, Python binding, packaged wheels, Windows installers, site, live
  monitoring and Linux/macOS collectors are outside M2.
- WMI calls may be slower/unavailable in restricted environments. Missing
  values are `Unknown`/JSON `null`; users should never interpret them as zero.
- A display's resolution may be unknown on hybrid-GPU configurations where
  its ownership cannot be identified reliably. Likewise firmware details may
  be unavailable for spanned/dynamic disks or without device IOCTL access.
