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
| Reopen / explicit elevation | Choice 2 and `hwspec --disk -d --elevate`; UAC, same sections, child pauses | Native explicit elevation launched; child arguments inspected; choice 2 covered by policy tests |
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

## Implementation references

- [Microsoft ShellExecute structure and flags](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-shellexecuteinfow)
- [Microsoft storage failure prediction](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_predict_failure)
