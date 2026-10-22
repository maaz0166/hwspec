# M2 acceptance checklist (Windows)

The M2 source has portable unit tests, but **native Windows compilation and real
hardware values must be verified on Windows**. M2 acceptance in the project plan
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
| Windows version, x64 | Not yet verified | Not yet verified |
| MSVC build and CTest | Not yet verified | Not yet verified |
| JSON smoke script | Not yet verified | Not yet verified |
| Normal mode matches system UI | Not yet verified | Not yet verified |
| CPU detailed fields / Unknown cases | Not yet verified | Not yet verified |
| GPU / disk / network detailed fields | Not yet verified | Not yet verified |
| Battery capacity & health (laptop) | Not yet verified | Not yet verified |
| JSON redirected / no extra text | Not yet verified | Not yet verified |

## Intentional scope boundaries and limitations

- Admin elevation, admin prompts, `--elevate`, `--skip-admin` and disk SMART are
  M3 and have not been implemented. Detailed disk health prints an explicit
  M3 status, not a fabricated SMART reading.
- `--report`, Python binding, packaged wheels, Windows installers, site, live
  monitoring and Linux/macOS collectors are outside M2.
- WMI calls may be slower/unavailable in restricted environments. Missing
  values are `Unknown`/JSON `null`; users should never interpret them as zero.
- A display's resolution may be unknown on hybrid-GPU configurations where
  its ownership cannot be identified reliably. Likewise firmware details may
  be unavailable for spanned/dynamic disks or without device IOCTL access.
