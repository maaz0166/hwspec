# System Specs CLI: Project Plan

Oct 18, 2026 · @MuhammadMaaz

## Goal and scope

Version 1 is a Windows-only command-line tool that shows a PC's hardware specs in one command, so nobody has to dig through Settings.

- **In v1:** CPU, memory, OS, and an `all` view that combines them. GPU, disk, network and battery follow in v0.2.
- **Not in v1:** Linux and macOS, live monitoring, temperature sensors, a graphical interface, themes or logos.
- **Success:** `sysinfo --all` prints correct values that match Task Manager and System Information, and finishes in about a second (a target to measure, not a promise).

The angle: fastfetch already covers general system info on Windows, so this tool aims to be a small, spec-focused command with clean default output, a Python API, a shareable report, and clear handling of admin-only data.

## Commands and output

Sections are chosen with long flags, so several can be combined in one run, such as `sysinfo --cpu --mem`. A modifier changes how much each section shows: `sysinfo --battery` is the normal view, and `sysinfo --battery -d` adds extra detail for the battery only.

| Flag | Normal view | With `-d` adds | Ships |
| --- | --- | --- | --- |
| `--cpu` | Name, physical and logical cores, base clock | Max boost clock, current clock, cache sizes by level, vendor, family, model and stepping, instruction extensions, virtualization state | 0.1 |
| `--mem` | Total, used and available memory | Per-module slot, size, type, speed and manufacturer; page file size and use | 0.1 |
| `--os` | Name, version, build, architecture | Uptime, install date, build revision, computer name | 0.1 |
| `--all` | The normal view of every section | The detailed view of every section | 0.1 |
| `--gpu` | Adapter name and dedicated video memory | Every adapter; shared memory, vendor and device ID, driver version, current resolution and refresh rate | 0.2 |
| `--disk` | Drives, size, free space | Physical model, type (SSD or HDD), bus (NVMe, SATA, USB), firmware version, file system; health status (needs admin) | 0.2 |
| `--net` | Adapters and addresses | Link speed, adapter status, MAC address, gateway, DNS servers | 0.2 |
| `--battery` | Charge level and power state | Design and full-charge capacity in Wh, health percent, cycle count | 0.2 |
| `--report` | Shareable Markdown with serial numbers and MAC addresses removed | Not applicable | Later |

Modifiers: `-d` or `--detail`, `--json`, `--no-color`, `--skip-admin`, `--elevate`, `--version`, `--help`. A value that cannot be read prints `Unknown`; the tool never crashes or guesses.

Flags combine and can come in any order. With no section flag, `sysinfo` behaves like `--all`. The detail modifier is `-d` (long form `--detail`) rather than `--d`, because argument parsers such as CLI11 expect a single dash for a single letter and a double dash for a full word. `-d` arrives in v0.2, starting with the battery and the CPU. Every other section gets a detail view too, but some values, such as the maximum boost clock, are not available on every machine and print `Unknown` when no source reports them.

Default text output (placeholders in angle brackets):

```
CPU
  Name        <processor name>
  Cores       <physical> physical, <logical> logical
  Base clock  <value> GHz
```

Detail mode on the CPU:

```
$ sysinfo --cpu -d
CPU
  Name             <processor name>
  Vendor           <vendor>
  Cores            <physical> physical, <logical> logical
  Base clock       <value> GHz
  Max boost clock  <value> GHz, or Unknown
  Current clock    <value> GHz
  Cache            L1 <size>, L2 <size>, L3 <size>
  Family / model   <family>, <model>, stepping <stepping>
  Extensions       <instruction sets, such as AVX2>
  Virtualization   <enabled or disabled>
```

The maximum boost clock is the hardest value to read, because no single Windows source reports it on every CPU. Try CPUID leaf 0x16 and `CallNtPowerInformation` first, test on at least one Intel and one AMD machine, and print `Unknown` where nothing reports it.

Detail mode on the battery, which also shows the health calculation:

```
$ sysinfo --battery
Battery
  Charge           <percent>%
  State            <charging or on battery>

$ sysinfo --battery -d
Battery
  Charge           <percent>%
  State            <charging or on battery>
  Design capacity  <value> Wh
  Full charge      <value> Wh
  Health           <value>%
  Cycle count      <value, or Unknown>
```

Health compares what the battery holds when full today with what it held when new:

```latex
\text{health} = \frac{\text{full-charge capacity}}{\text{design capacity}} \times 100
```

Caveats:

- A PC without a battery prints `No battery detected`.
- Some batteries report capacity in relative units, or report a cycle count of 0. In those cases print `Unknown` for the affected values and skip the health figure rather than guess.
- With `--json`, `-d` adds the same fields to the `battery` object.

JSON output uses one object per section, and a section skipped for lack of admin rights says so:

```json
{"cpu": {"name": "<processor name>"}, "disk_health": {"status": "requires_admin"}}
```

## Architecture

One C++ core library does all the work, and the command-line program and the Python module are thin layers on top of it.

&#91;embedded content: architecture · 2 front ends over 1 core\]

Collectors never print anything; the CLI and the Python module both call them and format the result.

Repository layout:

```
sysinfo/
  core/
    include/sysinfo/      public headers: cpu.hpp, mem.hpp, ...
    src/windows/          Windows collectors: cpu.cpp, mem.cpp, ...
  cli/                    main(), argument parsing, text and JSON formatters
  python/                 binding code and pyproject.toml
  tests/
  .github/workflows/
```

Rules that keep it clean:

- Collectors return plain structs and never print.
- All command-line logic lives in one function, `run(argc, argv)`, so the `.exe` and the pip command behave identically.
- Windows code stays under `src/windows/` behind the public headers, so other operating systems can be added later without touching the CLI.

Default tools: CMake with MSVC, CLI11 for arguments, nlohmann/json for JSON, pybind11 for the Python binding, GoogleTest or Catch2 for tests.

## Windows data sources

Most v1 data comes from plain Win32 calls and the registry, and almost none of it needs admin rights; only disk health, sensors and security state do.

| Data | Windows source | Needs admin |
| --- | --- | --- |
| CPU name | `__cpuid` brand string, or registry `HARDWARE\DESCRIPTION\System\CentralProcessor` | No |
| CPU cores and cache | `GetLogicalProcessorInformationEx` | No |
| CPU base and current clock | Registry `~MHz` value (base); PDH `Processor Information` counters (current) | No |
| CPU max boost clock | `__cpuid` leaf 0x16 on many Intel CPUs; `CallNtPowerInformation` with `ProcessorInformation`; not reported on every CPU | No |
| CPU vendor, family, model, stepping, extensions, virtualization | `__cpuid`, `IsProcessorFeaturePresent` | No |
| Memory total, available and page file | `GlobalMemoryStatusEx` | No |
| RAM modules (type, speed, slots, manufacturer) | SMBIOS via `GetSystemFirmwareTable`, or WMI `Win32_PhysicalMemory` | Likely no |
| OS name, build, uptime | Registry `CurrentVersion` key, `GetTickCount64` | No |
| OS install date, build revision, computer name | Registry `InstallDate` and `UBR` values, `GetComputerNameEx` | No |
| GPU name, video memory, vendor and device ID | DXGI `EnumAdapters1` | No |
| GPU driver version | WMI `Win32_VideoController` | Likely no |
| Display resolution and refresh rate | `EnumDisplaySettings` | No |
| Drive size and free space | `GetLogicalDrives`, `GetDiskFreeSpaceEx` | No |
| File system per drive | `GetVolumeInformation` | No |
| Physical disk model, type, bus, firmware | `IOCTL_STORAGE_QUERY_PROPERTY` | Likely no |
| Network adapters, speed, MAC, gateway, DNS | `GetAdaptersAddresses` | No |
| Battery charge and power state | `GetSystemPowerStatus` | No |
| Battery capacity, health, cycle count | Battery IOCTLs (`IOCTL_BATTERY_QUERY_INFORMATION`), or WMI classes `BatteryStaticData`, `BatteryFullChargedCapacity` and `BatteryCycleCount` in `root\wmi` | Likely no |
| Disk health (SMART) | WMI storage classes in `root\wmi` | Yes |
| TPM and BitLocker state | WMI or PowerShell cmdlets | Yes |
| CPU and GPU temperature, fans | Needs a kernel driver, as hardware-monitor tools use | Yes, and out of scope |

Treat every row marked Likely no as unverified until you test it as a standard user. Start with Win32 and the registry, and add WMI only for data that has no simpler source, because it needs COM and is slower.

## Admin-rights design

The tool never fails or hangs on a missing permission: it runs the section, skips it with a clear note, or relaunches elevated, and the user chooses.

| Situation | Behavior |
| --- | --- |
| Console already elevated | Every section runs; no prompt |
| Not elevated, requested sections need no admin | Runs normally; no prompt |
| Not elevated, interactive terminal, some sections need admin | Shows the prompt below |
| Not elevated, output piped, or `--json`, or `--skip-admin` | Never prompts; skips admin-only sections and marks them |
| `--elevate` | Relaunches as administrator immediately |

The prompt:

```
2 sections need administrator rights: disk health, TPM.
  [1] Skip them and continue (default)
  [2] Reopen this command as administrator
  [3] Cancel
Choice [1]:
```

For example, `sysinfo --disk` never prompts, but `sysinfo --disk -d` includes disk health, which needs admin, so a standard user in a terminal sees this prompt.

Implementation notes:

- Detect elevation with `OpenProcessToken` and `GetTokenInformation(TokenElevation)`; detect an interactive terminal with `_isatty` on stdin and stdout.
- Every collector declares `requires_admin`, so the CLI knows which sections to flag before it reads anything.
- **Reopen** uses `ShellExecuteEx` with the `runas` verb and the same arguments. Windows shows the UAC dialog and opens a new console window, and output from the first window does not carry over.
- That new window closes when the command ends, so pass `--pause` to the relaunched command to keep it open. If the user declines the UAC dialog, fall back to skipping.
- Skipped data is marked in text output (`Disk health: requires admin`) and in JSON (`status: requires_admin`), so scripts can tell skipped from unknown.
- Later, add a config setting `admin_prompt = ask | skip | elevate` so `all` does not ask every time.

Build the `requires_admin` flag into the collector interface in M1, even though the prompt itself arrives in M3.

## Milestones

The project runs in seven steps, with the Windows core usable by M1 and the website last. Effort is relative (S, M, L) because your weekly time is unknown.

| Milestone | Deliverable | Done when | Effort |
| --- | --- | --- | --- |
| M0 Setup | Repository, CMake and MSVC build, GitHub Actions on `windows-latest`, licence, name chosen and reserved on PyPI with a first small release, private repository | CI builds a hello-world `.exe` | S |
| M1 MVP | Core library plus `--cpu`, `--mem`, `--os`, `--all`; text output; unit tests for formatters; `requires_admin` flag in the collector interface | Values match Task Manager and System Information on your own PC | M |
| M2 More data | `--json`; `--gpu`, `--disk`, `--net`, `--battery`; the `-d` detail views for every section, battery and CPU first; `Unknown` handling | Every section and its `-d` view run on at least two different PCs; `--battery -d` shows capacity and health on a laptop | M to L |
| M3 Admin handling | Elevation check, prompt, `--skip-admin`, `--elevate`, `--pause`; first admin-only data: disk health under `--disk -d` | Same command behaves correctly as admin, as a standard user, and piped | M |
| M4 Python package | pybind11 binding; wheels for Windows x64 built with cibuildwheel; TestPyPI, then PyPI | `pip install` in a clean virtual environment gives a working command | M to L |
| M5 Release | `.exe` and zip on GitHub Releases, Inno Setup installer, winget manifest, README with screenshots | Someone without Python installs it and runs it | M |
| M6 Later | Website, `report`, live `watch` mode, config file, Linux and macOS backends | Decided after M5 | L |

## Testing

Because collectors return plain data, most of the tool can be tested without any real hardware.

- **Unit tests:** feed fake data to the text and JSON formatters, and check the argument parser and the admin decision table (GoogleTest or Catch2).
- **Reality check:** compare real output with Task Manager, System Information (`msinfo32`) and a tool such as CPU-Z on your own PC.
- **Machine spread:** run on at least a laptop, a desktop and a virtual machine, and on both Windows 10 and 11. Virtual machines often report fewer cores and no battery, which exercises the `Unknown` path.
- **Three modes:** run every command as administrator, as a standard user, and piped to a file.
- **CI on `windows-latest`:** build, run unit tests, and smoke-test `sysinfo --all --json` by checking that the output parses. Hardware values differ on CI machines, so check the shape, not the numbers.

## Distribution

Ship pip first, then a GitHub Releases `.exe`, and put the website last, because the website only links to the first two.

| Channel | How | Notes |
| --- | --- | --- |
| pip | Windows x64 wheels built with cibuildwheel in GitHub Actions | The pip command calls the same C++ `run(argc, argv)` function as the `.exe`, so both behave identically |
| GitHub Releases | `.exe` and a zip attached to each version tag, built in CI | The base for every other channel |
| Installer | Inno Setup wrapper that installs the `.exe` and adds it to PATH | After M5 basics work |
| winget and scoop | Manifests that point at the release assets | An install command without needing a website |
| Website | Static page on GitHub Pages with download and pip instructions | After M5 |

An unsigned `.exe` downloaded in a browser usually triggers a Windows SmartScreen warning, while pip installs usually avoid it. Code signing can wait until people ask for it.

While the repository is private, GitHub Releases downloads are not public and winget needs public download URLs, so make the repository public, or publish releases from a separate public repository, before M5. Publishing to PyPI does not depend on the repository being public. Private repositories also get a limited monthly allowance of free GitHub Actions minutes, and Windows runners use it faster than Linux ones, so keep CI runs short until then.

## Risks and honest notes

The C++ is the easy half; the competition, the name and the packaging are what can stall the project.

- **Competition:** [fastfetch](https://github.com/fastfetch-cli/fastfetch/wiki/Home) already supports Windows and is actively maintained, and a [`sysinfo` package](https://pypi.org/project/sysinfo) already exists on PyPI. A JSON option alone will not set this tool apart; the angle in Goal and scope has to.
- **Name clash:** `sysinfo` is taken on PyPI and sits next to Windows' built-in `systeminfo` command. Pick a different name before M0 ends.
- **Packaging time:** Wheels, an installer and a release pipeline usually take longer than the collectors themselves.
- **Number accuracy:** Different Windows APIs report slightly different memory and clock values, so choose one source per value and test it against Task Manager.
- **VMs and WSL:** Values differ inside virtual machines, and the Windows `.exe` run from WSL reports the Windows host, not the Linux side.
- **Scope creep:** Temperatures, live monitoring and a GUI each need drivers or extra design. Keep them out until v1 ships.

## Open decisions

This plan assumes the defaults below; change any of them and the plan follows.

| Decision | Default assumed | Note |
| --- | --- | --- |
| Final name | Final, chosen 18 Oct 2026: `hwspec` | Examples in this plan still use `sysinfo` as a placeholder |
| Command style | Long section flags (`sysinfo --cpu --mem`) with `-d` or `--detail` as a modifier | Flags let several sections combine in one run; subcommands (`sysinfo cpu`) were the earlier suggestion |
| Licence and visibility | MIT licence; private repository at first | Make it public before M5, see Distribution |
| Windows support | Windows 10 and 11, x64 only | ARM64 later if people ask |
| Python binding | pybind11 | nanobind is the newer alternative |
| Colour | On in a terminal, off when piped or with `--no-color` | Matches common CLI behavior |

Name check on 18 Oct 2026: `hwspec` is free on PyPI and no existing tool turned up in a web search, so it is the front-runner. GitHub could not be checked from here, so search github.com for each name before choosing.

| Name | PyPI | Notes |
| --- | --- | --- |
| `sysinfo` | Taken | A [package of that name](https://pypi.org/project/sysinfo) exists |
| `specs`, `spec`, `sysfacts`, `specsheet` | Taken | Skip |
| `hwspec` | Free | No existing tool found; front-runner |
| `pcspec`, `pcspecs` | Free | A [Rust crate named pcspecs](https://docs.rs/crate/pcspecs/0.1.1) already exists; too close |
| `syspeek` | Free | [An existing system monitor](https://github.com/vicox/syspeek) uses this name |
| `sysglance` | Free | [An existing Linux app](https://www.onworks.net/software/linux/app-sysglance?amp=1) uses this name |
| `machinfo` | Free | I believe it is also an HP-UX command (from memory, unverified) |
| `pcprobe` | Free | ASUS has a utility called PC Probe (from memory, unverified) |
| `sysspec`, `sysspecs`, `hwspecs`, `rigspec`, `rigcheck`, `sysbrief`, `sysdetail` | Free | Checked on PyPI only |

Next steps for the final name `hwspec`: search github.com for it, run `winget search hwspec`, and register it on PyPI with a small first release.
