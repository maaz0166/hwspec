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
| MSVC build and CTest | MinGW build + CTest pass; MSVC pending | Not yet verified |
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

The table above includes partial acceptance results: UI comparisons, MSVC,
a second physical PC, and Intel/AMD coverage are still pending. CIM comparisons
do not claim Task Manager/System Information UI validation.

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
