# hwspec

Windows 10/11 x64 hardware specifications, backed by the same C++ core as the
standalone executable. Python 3.9+ on Windows x64 is required. Version 0.4.0
implements the native binding; publication to TestPyPI/PyPI is a separate step.

## Command line

```powershell
hwspec --cpu --mem
hwspec --all -d --json
python -m hwspec --battery -d
hwspec --disk -d --skip-admin
```

The pip command and `python -m hwspec` call the shared C++ CLI entry point.
Section flags combine; no section means all sections. `-d` adds detail and
`--json` outputs typed JSON, with `null` for missing values. Text uses `Unknown`.

Normal collection needs no administrator privileges. Detailed disk health may
need them: interactive text mode offers skip, reopen or cancel. JSON and piped
runs never prompt. `--elevate` explicitly reopens the current Python interpreter
with `-m hwspec` and the same flags, adding `--pause`. Results appear in the new
console; declining UAC continues with privileged data skipped. `--pause` only
waits in an interactive text console.

## Python API

```python
import hwspec

cpu = hwspec.collect("cpu")
details = hwspec.collect(["cpu", "mem", "battery"], detail=True)
everything = hwspec.collect()
disks = hwspec.collect("disk", detail=True, skip_admin=True)
print(cpu["cpu"]["name"])
```

`collect(sections=None, *, detail=False, skip_admin=False)` returns dictionaries,
lists, numbers, strings, booleans and `None`, using the CLI JSON schema. Section
names are `cpu`, `mem`, `os`, `gpu`, `disk`, `net`, `battery` and `all`. Empty
selection also means all. Unknown names raise `ValueError`; non-string names
raise `TypeError`. Library calls do not print, prompt, pause or request elevation.
Hardware collection releases Python's GIL. In an already elevated process,
detailed SMART collection is attempted unless `skip_admin=True`.

Unsupported hardware values stay unknown. Some NVMe/USB/RAID drivers do not
expose the legacy SMART WMI provider; `disk_health.status` is `unknown` after
an unsuccessful query, `requires_admin` when skipped, or `available` when a
device reports a prediction. No failure predicted does not guarantee health.

## Build from source

Build from the repository root, which contains the canonical packaging metadata:

```powershell
python -m pip install build
python -m build
```

Source builds need CMake and a Windows x64 C++ toolchain (MSVC recommended).
Binary wheels require neither a compiler nor CMake to install. Wheels are
configured for CPython 3.9–3.14 on Windows x64; other interpreters and Windows
ARM64/32-bit are outside the supported wheel matrix. The source distribution
includes the C++ core and CLI, so it can build independently of a Git checkout.
