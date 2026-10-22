# hwspec — Codex Project Instructions

## Project context

`hwspec` is a Windows-only, x64 command-line utility for displaying hardware and OS specifications. Its implementation is in C++ and uses Windows APIs.

The full project specification is maintained in `docs/project-plan.md`. **Read that plan and the relevant current source files before planning or implementing changes.** The plan is the reference for scope, milestone deliverables, commands, and acceptance criteria. When implementation and plan disagree, report the discrepancy rather than silently changing the intended scope.

## Architecture and code conventions

- Use one shared C++ core library for hardware collection.
- Keep Windows-specific collectors under `core/src/windows/`, behind public interfaces in `core/include/`.
- Collectors return plain data structures; they do not print or make CLI decisions.
- Keep argument parsing, command behavior, and output formatting in the CLI layer. Preserve the shared CLI entry point for future Python integration.
- Section flags can be combined. An omitted section flag means `--all`.
- `-d` / `--detail` adds detail fields to the selected sections, and `--json` produces machine-readable output.
- If a hardware value cannot be obtained reliably, represent it as `Unknown` in human-readable output and `null` in JSON. Never fabricate a value.
- Prefer Windows APIs and the registry where appropriate; use WMI only when necessary. Handle unsupported devices, missing data, and errors gracefully.
- Preserve existing behavior and public interfaces unless a requested change requires otherwise.

## Milestone status

- **M0 — Setup:** Complete.
- **M1 — MVP:** Complete (`--cpu`, `--mem`, `--os`, `--all`, initial tests).
- **M2 — More data:** Native MSVC/MinGW builds, seven CTests per toolchain, JSON smoke, battery report and 16 actual Task Manager UI comparisons passed on one Windows 11 laptop. UI comparison found and fixed cache totals (sum distinct records) and configured RAM speed (ConfiguredClockSpeed, not rated Speed). Updated runner passed 109 standard-user and 110 elevated checks. **Second-PC and broader Intel/AMD/Windows 10/VM coverage remain pending.** Source/native corrections postdate the published Python 0.4.0 artifacts. See `docs/M2_VALIDATION.md`.
- **M3 — Administrator handling:** Source implemented. Native standard/elevated/piped checks pass, including elevated skip behavior. Full native UAC integration verified the elevated token, preserved Disk arguments, pause prompt and exit 0 after Enter. This NVMe SMART provider is unsupported and correctly reports Unknown. **Actual user-observed UAC rejection remains pending; a fresh attempt relaunched successfully rather than demonstrating rejection.** See `docs/M3_VALIDATION.md`.
- **M4 — Python package:** Delivered and published (v0.4.0): pybind11 API, shared pip/module CLI, Python-aware elevation, source distribution and Windows wheel workflow. All six CPython 3.9–3.14 Windows x64 wheels passed local cibuildwheel build/repair and installed-package validation (15 tests per interpreter); CPython 3.12 also built from sdist. Live installed-package Python UAC preserved the virtual environment/arguments, displayed a pause and exited 0 after Enter. **Approved TestPyPI then PyPI publication succeeded; each registry passed a fresh CPython 3.12 installation, all 15 tests and pip check. All seven published file hashes match the tested artifacts. GitHub-hosted execution is not yet observed.** See `docs/M4_VALIDATION.md`.
- **M5 — Release/distribution:** Not implemented.
- **M6 — Later features:** Deferred.

Do not mark M2 fully verified just because portable tests pass. Confirm the current repository state and Git history rather than assuming this status note is always up to date.

## Development workflow

1. Read `docs/project-plan.md`, this file, and the relevant implementation and tests.
2. Review Git history and check the working tree before changing files.
3. Identify the requested milestone's scope, dependencies, and measurable acceptance criteria.
4. First explain the proposed approach and wait for approval before implementing a new milestone, unless the user explicitly asks you to proceed immediately.
5. Make focused, incremental changes, keeping existing milestones working.
6. Add or update tests for behavior, formatters, JSON structure, flag handling, and missing-data paths.
7. Build and run relevant checks where possible. On Windows, include real-device verification; on other platforms, clearly distinguish portable test results from untested native collectors.
8. Report what changed, commands run, tests passed/failed, remaining limitations, and anything that must be checked manually.
9. **Do not commit, push, create releases, publish packages, or modify GitHub state without explicit user permission.** Do not force-push or overwrite history.

## Current priority

M4 publication and fresh-registry installation checks are complete. The user then requested completion of M2/M3 acceptance: Task Manager comparison and native pause/Enter-to-close are now verified; second-PC results and actual UAC rejection still require user/external evidence before M5. The source/native cache and RAM corrections are not in the already published Python 0.4.0 wheels; a future package release needs its own build/validation/publication authorization. GitHub-hosted workflow results remain unobserved. Preserve completed verification. Use `tests/verify_windows.ps1` and `build/hwspec-m2-m3-validation.zip` for per-PC checks, `tests/verify_native_uac.ps1` for native elevation/console checks and `tests/verify_python_uac.ps1` for the Python path. Investigate failures without guessing unsupported values.

## Learnings

- CPU cache size is the total across distinct `RelationCache` records at each level, not the maximum individual record. Four cores with separate 32 KiB instruction/data L1 caches total 256 KiB; a shared L3 record is counted once, not multiplied by core count.
- `Win32_PhysicalMemory.Speed` can report a module's rated 2667 value while `ConfiguredClockSpeed` and Task Manager show the actual configured 2400 value. Use the configured property for the existing speed field; missing/zero values remain Unknown/null rather than falling back to a differently defined property.
- Native UAC command lines quote individual flags, so observer matching must accept `"-d"`. A console in selection mode (window title starts with Select) can block output; inspect selection state and resume only the verified test console, never send desktop-wide keys or treat an empty selected console as a collector failure.
- Windows headers can define `min`/`max` before a later helper sets `NOMINMAX`. Use `(std::numeric_limits<T>::max)()` when an existing include order exposes the Windows `max` macro; verify with MSVC as well as MinGW.
- Test comparisons must keep their operands alive. Copy values in `CHECK_EQ`: binding `const auto&` to `format_mem(info).fields[1].value` leaves a dangling reference after the temporary report is destroyed because vector indexing does not extend its lifetime.
- A negative `PF_VIRT_FIRMWARE_ENABLED` result under an active hypervisor does not reliably establish BIOS state. Preserve Unknown/null in that case; do not infer enabled firmware merely from the hypervisor either. Different hardware API properties (for example DXGI dedicated video memory versus WMI AdapterRAM) must retain their source distinctions when values disagree.
- Distribute cibuildwheel's repaired wheels, not raw `python -m build` wheels. Local Visual Studio runtime installations can hide missing DLL dependencies; the CPython 3.12 build needed `msvcp140.dll`, which delvewheel bundled. Root `pyproject.toml` must keep the CMake/package versions consistent, and sdists must include core, CLI and binding sources.

## Platform and tools

- Windows 10/11 x64 is the v1 target.
- CMake and a Windows-capable C++ toolchain (prefer MSVC) for native builds.
- Windows APIs and registry for data collection; the plan documents alternatives and limitations.
- Python integration uses pybind11 in M4. Root `pyproject.toml` is canonical so sdists contain the core and CLI; build from the repository root. The package is Windows x64 only.

Keep changes consistent with the project plan and the code already present in this repository.
