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
- **M2 — More data:** Native MSVC and MinGW builds, CTest, JSON smoke checks, CIM comparisons and laptop battery capacities verified on one Windows 11 laptop. **Manual UI comparisons and the second-PC acceptance checks remain pending.** See `docs/M2_VALIDATION.md`.
- **M3 — Administrator handling:** Source implemented. Native standard/elevated/piped checks pass, including elevated skip behavior. Explicit elevation preserved arguments and added pause; this NVMe SMART provider is unsupported and correctly reports Unknown. **User-observed UAC rejection and console close-on-Enter confirmation remain pending.** See `docs/M3_VALIDATION.md`.
- **M4 — Python package:** Source implemented (v0.4.0): pybind11 API, shared pip/module CLI, Python-aware elevation, source distribution and Windows wheel workflow. All six CPython 3.9–3.14 Windows x64 wheels passed local cibuildwheel build/repair and installed-package validation (15 tests per interpreter); CPython 3.12 also built from sdist. Live installed-package Python UAC preserved the virtual environment/arguments, displayed a pause and exited 0 after Enter. **TestPyPI/PyPI publication remains blocked on publishing access; GitHub-hosted execution is not yet observed.** See `docs/M4_VALIDATION.md`.
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

The user authorized proceeding with M4 on 2026-10-22 while external M2/M3 evidence remains pending. Finish the remaining acceptance work before M5: second-PC/Task Manager comparisons, user-observed native UAC/console behavior, the Python wheel matrix, and approved TestPyPI/PyPI publication. Preserve the completed local MSVC/MinGW/laptop, elevated SMART, Python wheel and installed-package UAC verification. Use `tests/verify_windows.ps1` for repeatable per-PC captures and `tests/verify_python_uac.ps1` for the Python elevation/console check. Investigate failures without guessing unsupported hardware values.

## Learnings

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
