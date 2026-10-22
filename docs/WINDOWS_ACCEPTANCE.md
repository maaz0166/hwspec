# Windows hardware acceptance

Use the same x64 Release executable on each PC. The runner requires Windows
PowerShell 5.1 or PowerShell 7, and does not require Python, CMake or a compiler.
It never requests elevation itself. To test administrator behavior, run it
again from a terminal explicitly opened as administrator.

From the repository:

```powershell
./tests/verify_windows.ps1 -Executable ./build/msvc/acceptance/hwspec.exe
```

Or extract the validation ZIP, open PowerShell in the extracted directory, and run:

The current refreshed kit is `build/hwspec-m2-m3-validation.zip`. It includes
the cache-total and configured-RAM-speed corrections made after Python 0.4.0
publication. Testing the older PyPI 0.4.0 wheel does not exercise those fixes;
the subsequently published Python 0.4.1 package includes them.

```powershell
powershell -NoProfile -File ./verify_windows.ps1 -Executable ./hwspec.exe -OutputDirectory ./results/standard
```

Repeat from an administrator PowerShell:

```powershell
powershell -NoProfile -File ./verify_windows.ps1 -Executable ./hwspec.exe -OutputDirectory ./results/elevated
```

Each invocation creates a separate timestamped directory with `summary.json`,
per-command stdout/stderr captures, and a SMART reference query result. The
summary includes the executable SHA-256, machine model, OS build, elevation
state, battery data and individual check results. The runner exits 1 if an
assertion fails; a timeout or collection/JSON error also stops with an error.
Raw captures include machine names, network addresses and device identifiers.
The compact summaries can be returned separately from those raw captures.

The runner exercises every section in normal/detailed text and JSON, all-section
selection, default output, help/version, invalid arguments, piped `--pause`,
`--skip-admin`, and the privilege-dependent disk health state. It compares CPU
name/core counts, memory and OS build with Windows CIM/registry data. When
elevated it compares SMART predictions with an independent CIM query; if the
provider is unsupported, `unknown` is expected rather than an invented result.
These are real CLI/hardware checks; they do not replace the manual checks below.

## Manual checks on each PC

1. Run `./hwspec.exe --all` and `./hwspec.exe --all -d --skip-admin`.
   Compare CPU cores, memory, OS, GPU, disk and network with Task Manager and
   System Information (`msinfo32`). Record mismatches, including the source
   and units; current clocks and free memory naturally change between samples.
2. On a laptop, compare `./hwspec.exe --battery -d` with a Windows battery report
   (`powercfg /batteryreport`). Record unsupported cycle/capacity values.
3. In a standard-user console run `./hwspec.exe --disk -d`:
   Enter/1 skips health; 3 cancels (exit 1); 2 opens UAC. Declining UAC must
   continue in the original console with disk health requiring admin.
4. Run `./hwspec.exe --disk -d --elevate` and approve UAC. The new console must
   show only Disk, avoid an admin prompt, report a SMART prediction or Unknown,
   and wait at `Press Enter to close...`. Confirm the console closes on Enter.
5. Run `./hwspec.exe --disk -d --json > disk.json` as both standard user and
   administrator. The file must contain JSON with no prompt. Use the runner's
   corresponding captures to check the JSON without installing another tool.

Return both summaries and the manual observations with the PC model and Windows
version. A second run on the same PC does not satisfy the two-PC requirement.
See [M2 validation](./M2_VALIDATION.md) and [M3 validation](./M3_VALIDATION.md).
