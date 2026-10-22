# M3 administrator handling

M3 source is implemented in v0.3.0. Automated decision-table tests and a native
Windows MinGW build do not replace interactive UAC and elevated hardware checks.
M2's second-PC acceptance check remains pending; see [M2 validation](./M2_VALIDATION.md).

## Behavior

- Normal sections run without prompting, including `--disk`.
- `--disk -d` (also `--all -d` or `-d` alone) requests disk SMART failure prediction.
- Standard users in an interactive text console can skip (default, including
  empty input/EOF/unrecognized input), reopen elevated, or cancel (exit 1).
- JSON, redirected input/output and `--skip-admin` never show the admin prompt.
- `--elevate` explicitly requests a Windows UAC relaunch even with JSON or piped
  output. Output appears in the new window; it does not feed the original pipe.
- Relaunch removes `--elevate` and duplicates of `--pause`, and appends one
  `--pause`. It uses the executable's absolute Unicode path and quoted arguments.
- If UAC is declined or relaunch fails, collection continues without privileged
  data, with a diagnostic on stderr. Successful launch exits the parent with 0;
  this indicates launch, rather than the child command's result.
- `--pause` waits for Enter only in interactive text mode, after flushing output.
  It never blocks JSON or redirected runs.
- Elevated runs collect SMART without a prompt. `--skip-admin` skips privileged
  data even when elevated; combining it with `--elevate` still skips that data
  in the elevated child.

## Disk health schema

Detailed JSON retains `disk.disk_health`, replacing M2's `not_implemented` marker:

```json
{"disk_health":{"status":"requires_admin","devices":[]}}
```

`requires_admin` means the privileged query was skipped. `unknown` means it was
attempted but no usable prediction was reported (unsupported device/provider,
denied access, missing data or another WMI failure). `available` means at least
one active physical-device instance supplied a boolean prediction:

```json
{"disk_health":{"status":"available","devices":[{"instance_name":"example device","failure_predicted":false}]}}
```

Missing properties remain `null`; text displays `Unknown`. A false prediction
means **No failure predicted**, rather than a guarantee of disk health. Active
instances come from `MSStorageDriver_FailurePredictStatus` in `ROOT\WMI` and
are reported independently of logical drives. They are never matched by array
order or attributed to a volume without evidence. NVMe/USB/RAID drivers may not
expose this legacy SMART provider. No health percentage is invented.

## Automated and native verification

On 2026-10-22, Windows 11 Pro x64, Dell Latitude 5500, standard-user process:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
python tests/smoke_json.py ./build/hwspec.exe
./build/hwspec.exe --all -d --skip-admin
./build/hwspec.exe --disk -d --json
```

The Release build passed, all seven CTest executables passed, and the JSON
smoke script passed. Real standard-user console tests verified the default skip,
cancel (exit 1, no collected output), and `--pause` waiting for Enter. Piped text
and JSON ran without prompting or pausing.

The admin unit tests cover all combinations of elevated/standard user,
interactive/piped, JSON/text and `--skip-admin`; ordinary section selection;
cancel; relaunch success/failure/unavailable; explicit elevation; pause guards;
whole-section privilege requirements; Windows argument quoting; and SMART true,
false and missing values. Process operations are injected, so unit tests do not
open UAC dialogs or require changing the test runner's privileges.

## Manual acceptance still required

Run the same native executable in these contexts and record actual results:

| Context | Commands / expected behavior | Status |
| --- | --- | --- |
| Standard user, piped | `hwspec --disk -d > disk.txt`; drives plus requires admin; no prompt | Tested locally |
| Standard user, JSON | `hwspec --disk -d --json`; valid JSON, requires_admin | Tested locally |
| Standard user, interactive | `hwspec --disk -d`; blank skips; 3 cancels | Tested locally in native console; explicit 1 covered by policy tests |
| Interactive pause | `hwspec --cpu --pause`; output then Enter to close | Tested locally in native console |
| Reopen / explicit elevation | Choice 2 and `hwspec --disk -d --elevate`; UAC, same sections, child pauses | Automated policy tests; native UAC check pending |
| UAC rejection | Decline dialog; parent continues with requires admin | Automated fallback test; native dialog check pending |
| Elevated, JSON and text | No prompt; prediction booleans or Unknown on unsupported hardware | Automated policy tests; native elevated check pending |
| Elevated skip | `hwspec --disk -d --skip-admin --json`; requires_admin | Automated policy test; native elevated check pending |

MSVC compilation is covered by the existing Windows CI workflow when it runs;
the local toolchain is MinGW-w64 GCC 15.2. No MSVC run is claimed here.

## Implementation references

- [Microsoft ShellExecute structure and flags](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-shellexecuteinfow)
- [Microsoft storage failure prediction](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_predict_failure)
