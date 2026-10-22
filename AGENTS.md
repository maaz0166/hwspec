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
- **M2 — More data:** Native MinGW build, CTest, JSON smoke checks, CIM comparisons and laptop battery capacities verified on one Windows 11 laptop. **MSVC, manual UI comparisons and the second-PC acceptance checks remain pending.** See `docs/M2_VALIDATION.md`.
- **M3 — Administrator handling:** Source implemented. Elevation detection, admin prompt, `--skip-admin`, `--elevate`, `--pause`, and WMI SMART prediction are present. Automated decision-table tests and native standard-user skip/cancel/pause checks pass. **Live UAC and elevated hardware acceptance remain pending.** See `docs/M3_VALIDATION.md`.
- **M4 — Python package:** Not implemented.
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

Finish the remaining M2/M3 acceptance checks before beginning M4: test a second physical PC, compare values with Task Manager/System Information, build with MSVC, and exercise live UAC approval/rejection and elevated SMART collection. Preserve the completed local MinGW/laptop verification recorded in the validation documents. Investigate failures without guessing unsupported hardware values.

## Platform and tools

- Windows 10/11 x64 is the v1 target.
- CMake and a Windows-capable C++ toolchain (prefer MSVC) for native builds.
- Windows APIs and registry for data collection; the plan documents alternatives and limitations.
- Python integration via pybind11 is planned for M4, not part of M2.

Keep changes consistent with the project plan and the code already present in this repository.
