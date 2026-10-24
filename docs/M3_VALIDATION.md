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
| Reopen / explicit elevation | Choice 2 and `hwspec --disk -d --elevate`; UAC, same sections, child pauses | Native explicit elevation/console/Enter-to-close passed live integration test; choice 2 covered by policy tests |
| UAC rejection | Decline dialog; parent continues with requires admin | Native fallback returned 0 with requires admin; user confirmation of No versus unavailable elevation pending |
| Elevated, JSON and text | No prompt; prediction booleans or Unknown on unsupported hardware | Native elevated piped text and JSON passed; this device/provider is unsupported and reports Unknown |
| Elevated skip | `hwspec --disk -d --skip-admin --json`; requires_admin | Native elevated check passed |

Local MSVC 19.44.35217.0 / Windows SDK 10.0.26100.0 x64 Release compilation,
all seven CTest executables, JSON smoke checks and detailed text output passed
on 2026-10-22 after fixing the Windows `max` macro collision and test operand
lifetimes described in [M2 validation](./M2_VALIDATION.md). MinGW-w64 GCC 15.2
was also exercised locally. The live follow-up below adds elevated verification;
user-observed dialog and console details still need confirmation.

## Live elevation follow-up: 2026-10-22

The same MSVC Release executable was exercised by
[verify_windows.ps1](../tests/verify_windows.ps1) as standard user (107 checks)
and administrator (108 checks), with zero failures in both contexts. The
SHA-256 in both summaries is
`593C3236D22FF73C694FF38B029A88228D01BAFFFEDFB5E45131D050BC8D31A3`.
The runner captures each section's normal/detailed text and JSON, skipped health,
piped pause behavior, and reference CIM comparisons.

- The first native `--disk -d --elevate` attempt returned 0, printed the
  elevation-declined-or-unavailable diagnostic, and continued with ordinary disk
  details plus `requires admin`. The user was instructed to choose No. Because
  the diagnostic also covers unavailable elevation, actual No-dialog observation
  remains unconfirmed rather than being assumed.
- A subsequent native `--disk -d --elevate` returned 0 with no parent output.
  An elevated observer found the child running the same executable with
  exactly `--disk -d --pause`, confirming section preservation, loop prevention
  and pause argument insertion. Closing on Enter still needs user observation.
- The separate elevated runner confirmed its administrator token. Native
  detailed text/JSON had no permission prompt. SMART was `unknown`, while an
  independent elevated `Get-CimInstance` query of the same provider returned
  `Not supported`. No healthy/failing prediction can be claimed for this NVMe.
- `--skip-admin --pause --json` remained noninteractive and returned
  `requires_admin` even when elevated, with no prediction records.
- The extracted validation kit passed the standard-user checks under Windows
  PowerShell 5.1 as well as the repository runner under PowerShell 7.

The raw local captures are under the ignored build directory. See
[Windows acceptance](./WINDOWS_ACCEPTANCE.md) for second-PC and interactive checks.

After the CPU virtualization correctness fix documented in M2 validation, the
corrected executable in `build/msvc/verified/` passed 108 standard-user checks
and 109 elevated checks, including the new hypervisor consistency assertion.
Both summaries identify SHA-256
`0CFCF613B4B91ECA0E47E9A8093CB6E4439E4E7092ACD5ADBBFDA8446D5D2373`.
Virtualization is now null on this hypervisor host; elevated SMART remains
unknown with the independent provider query reporting unsupported. This is
the executable included in the final validation kit.

## Completed native console acceptance follow-up: 2026-10-22

[verify_native_uac.ps1](../tests/verify_native_uac.ps1) passed against the corrected
MSVC executable under `build/msvc/acceptance/`, SHA-256
`335B9755CEE1AF76E5BCD3E7B7310FC5605B14DE4C8C7B207C7A3223D6FBC5A3`:

```powershell
./tests/verify_native_uac.ps1 -Executable ./build/msvc/acceptance/hwspec.exe -Python ./build/python-build-env/Scripts/python.exe
```

The standard-user parent returned 0 with empty stdout/stderr. An elevated
observer checked the exact executable and preserved `--disk -d --pause`
arguments. Its console inspector verified an elevated token, only Disk output,
Unknown SMART health, and `Press Enter to close...`; it sent Enter to that
console's input buffer and verified exit 0. Results are under
`build/native-uac-validation/20261022-122746-210/`. No desktop-wide key input
was used. The corrected build also passed the updated 109 standard-user and
110 elevated acceptance checks.

Earlier observer attempts are retained as failures: one missed the quoted
`"-d"` argument; another encountered an empty console while Windows selection
mode paused output. The helper now matches quoted flags and detects selection
mode. The native runner explicitly allows Escape only to its verified test
console before inspecting it. A recovery check passed, followed by the full
corrected-build test above with no selection recovery needed. These were test
observer issues, not evidence of an application collection failure.

A new rejection attempt instead returned a successful launch (empty parent
output), so it did **not** verify UAC rejection. Its results are retained under
`build/native-uac-rejection/`; user confirmation of actually choosing No remains
pending. Do not mark M3 completely accepted on that evidence. The previous
fallback result and automated policy tests remain valid but do not establish
an observed No-dialog interaction.

## Implementation references

- [Microsoft ShellExecute structure and flags](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-shellexecuteinfow)
- [Microsoft storage failure prediction](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_predict_failure)

## Follow-up: administrator menu input (local fix, not released)

A user reported that a friend selected option 2 after previously selecting
option 1, but still received output without administrator data. The exact
prompt input, executable version and remote terminal output have not yet been
captured, so the cause on that PC is not confirmed.

Inspection found a reproducible input-reader defect: only an exact `2` or `3`
was recognized; surrounding whitespace or invalid text silently selected skip.
The previous decision-table tests injected a choice and did not exercise the
real stdin reader. New tests use that reader with controlled input and a fake
relauncher (no UAC dialog): exact choices, surrounding whitespace/CRLF, invalid
input followed by a valid choice, cancellation, Enter/EOF defaults and successive
choices. They failed before the fix and now pass. Invalid input now prompts
again, whitespace is accepted, and Enter/EOF retain the safe skip default.

Both native MSVC static-runtime and MinGW builds passed all seven CTests.
Real-device `--disk -d --skip-admin` text and JSON retained the explicit
`requires admin` / `requires_admin` disk-health status. No new user-observed UAC
acceptance or rejection was performed for this change. The published 0.5.0
release assets remain unchanged; this fix is local and not committed/published.

For remote diagnosis, request `hwspec --version`, the exact prompt/output and
whether `hwspec --disk -d --elevate` produces a Windows permission prompt or a
new console. Successful elevation can still yield Unknown health when the disk
or driver does not expose the supported SMART provider.

The reporter subsequently clarified that the friend installed through pip,
not the standalone GitHub download. Diagnose the installed Python package and
interpreter first; the published PyPI version is 0.4.1. Both frontends share the
menu reader, so the input defect also exists in the published Python source,
but the friend's exact failure remains unconfirmed. The local regression tests
above cover the shared C++ path, not an installed wheel with this new fix.
Compare `hwspec --version` with `python -m hwspec --version`, then try
`python -m hwspec --disk -d --elevate` using the same Python environment.

## 0.5.1 publication follow-up

The administrator-menu fix is now published on GitHub, TestPyPI and PyPI.
[0.5.1 validation](./V0_5_1_VALIDATION.md) records the real pip-console reproduction,
fixed-reader checks, hosted builds, fresh registry installations and final hashes.
Final PyPI install verification completed on 2026-10-24. Earlier results and
immutable artifacts recorded above remain historical evidence; pending-publication
statements above describe the state before this follow-up. Broader hardware and
actual user-observed UAC rejection remain open.
