# 0.5.1 administrator-menu fix and release verification

## Scope and authorization

The user reported a pip-installed CLI behaving as if option 1 had been selected
when option 2 was requested. The user authorized correcting the possible cause
and republishing to GitHub, TestPyPI and PyPI. This release uses 0.5.1 across
native and Python metadata. Original 0.4.0/0.4.1 Python and 0.5.0 native assets
must remain unchanged. The unrelated LinkedIn draft is not part of this release.

## Reproduction and fix

The original reader compared the whole `std::getline` result with `2` or `3`.
CPython configures Windows C stdio in binary mode, so a console CRLF line can
leave a carriage return in that string. An exact menu choice could therefore
be silently converted to skip. Surrounding spaces had the same effect.

A fresh console using the installed PyPI 0.4.1 CPython 3.12 package reproduced
this: entering `3` displayed the disk report and returned 0 instead of cancelling.
The first `1` check succeeded. Evidence: `build/v051-old-prompt.json`.
This reproduces a relevant pip defect; it does not prove the remote reporter's
exact version, terminal, policy or hardware conditions.

The shared reader now strips surrounding ASCII spaces/tabs/CR/LF, accepts only
single choices 1–3, and asks again for invalid input. Empty input or EOF safely
skips. No process-wide Python stdio settings are changed. Disk-health availability
still depends on permissions and device/driver support.

## Local verification before publication

- Added C++ tests exercise the actual stdin reader, with a fake relauncher:
  exact/CRLF/spaced choices, option 2 preserved arguments, invalid-input retry,
  cancellation, Enter/EOF defaults and successive choices. They failed before
  the fix and passed afterward. MSVC and MinGW each passed seven CTests.
- Rebuilt native MSVC version 0.5.1; all seven CTests passed.
- Built an MSVC CPython 3.12 development wheel. A fresh environment passed all
  16 installed-package tests, including parity with the native executable.
  This unrepaired development wheel is not a publication artifact.
- `tests/verify_console_prompt.py` creates a hidden console for its own child,
  reads its buffer and writes scoped input events. No global keyboard input is
  used. The native executable, Python module and pip console entry point each
  passed four real-console checks: skip, exact cancel, spaced cancel and retry
  after invalid input. Evidence: `build/v051-{native,new,entry}-prompt.json`.
- This console helper does not select elevation or claim a newly observed UAC
  rejection. Existing native/Python UAC verification remains historical evidence.

## Publication procedure and status

Publication and verification completed; final PyPI installation verification was
performed on 2026-10-24. Release source commit: `efa4f79fa5eb3b79a1d12a107fb97734c3367800`.

- [Native release workflow](https://github.com/maaz0166/hwspec/actions/runs/39843205107)
  passed seven CTests, six distribution checks against each standalone/portable/
  installed binary, winget validation and 28 installer lifecycle checks. It
  published [v0.5.1](https://github.com/maaz0166/hwspec/releases/tag/v0.5.1) as a
  public prerelease with five assets. All downloads were fetched without
  authentication and matched GitHub digests and the published checksum manifest.
  The downloaded portable archive passed six smoke checks; the downloaded native
  executable passed all four real-console prompt checks.
- [Python wheel workflow](https://github.com/maaz0166/hwspec/actions/runs/39843204439)
  built six repaired CPython 3.9–3.14 x64 wheels from the same frozen sdist.
  All 16 tests passed on every wheel (96 test executions, no skips); the native
  parity executable was available to both parity tests. Seven metadata checks
  passed. Only those repaired CI artifacts were distributed.
- [Ordinary native CI](https://github.com/maaz0166/hwspec/actions/runs/39843204425)
  also completed successfully for the release source commit.
- [TestPyPI 0.5.1](https://test.pypi.org/project/hwspec/0.5.1/) was published first.
  A fresh CPython 3.12 environment installed from its normal index without cache,
  source compilation or dependencies, then passed all 16 package tests, `pip check`
  and four real-console checks each through the module and pip entry point.
- Only after those checks passed were the identical seven files uploaded to
  [PyPI 0.5.1](https://pypi.org/project/hwspec/0.5.1/). A separate fresh environment
  installed from the normal PyPI index and passed the same 16 tests, `pip check`
  and both sets of four real-console checks. Tests ran outside the repository.
- All seven Python artifact hashes on each registry exactly match the tested
  local CI downloads. Early pip lookups preceded simple-index propagation and
  reported only older versions; retrying installation after index propagation
  succeeded. No duplicate upload or overwrite was attempted.

Evidence remains in `build/v051-*`, including registry file hashes, pip install
reports, console-probe reports and CI logs. Downloaded Python artifacts are in
`dist/0.5.1/`; native public downloads are in `build/v051-native-public/`.
Earlier source/package/release artifacts remain unchanged. The winget manifest
is validated and published but has not been submitted to the community catalog.

## Published checksums

### Python (identical on TestPyPI and PyPI)

| File | SHA-256 |
| --- | --- |
| `hwspec-0.5.1-cp310-cp310-win_amd64.whl` | `d2436a0b0c6c1be6bfe4c6bf481609438b0b51ac1a5165dd2c894d74f70dab36` |
| `hwspec-0.5.1-cp311-cp311-win_amd64.whl` | `6409c5d9f1607eeb10bf132358e44b448d6fb7e956edcdc77391ad49d287d825` |
| `hwspec-0.5.1-cp312-cp312-win_amd64.whl` | `379868ebf11a0f8a0286fece50ba0e6ad8942106239d9269bb1c57212e8463d8` |
| `hwspec-0.5.1-cp313-cp313-win_amd64.whl` | `d37a12aecab7f4376a57861a004a3cc58fa4c8faa3436204bd07f77e754a4946` |
| `hwspec-0.5.1-cp314-cp314-win_amd64.whl` | `58319d24968177568e40d670dd75f8e352c0dad463b4bce6d8a7f95927944ffb` |
| `hwspec-0.5.1-cp39-cp39-win_amd64.whl` | `c1b76b5315901e9e7ef8086baf442c645eb799440f12e5bae222b26ecec1ee30` |
| `hwspec-0.5.1.tar.gz` | `8731e763438a3f5ac556e10497871b6f9a9f97009a5b7e1144a2e61a0de0eded` |

### Native GitHub assets

| File | SHA-256 |
| --- | --- |
| `hwspec-0.5.1-windows-x64.exe` | `eadd4e96b9619b1e90ce05d790f5a56f1cd4c3dae4b7760f277f0edfd54bf9ce` |
| `hwspec-0.5.1-windows-x64.zip` | `ca5b6be993518285d48175a5feaabbe3b6c62558c14e08eb1ff21c4674ed2a42` |
| `hwspec-0.5.1-windows-x64-setup.exe` | `c3fa1548ccaa5144ff071cb220b075f72bff97111f72a5d5ee9282101d71e523` |
| `hwspec-0.5.1-winget.zip` | `daf90471048e053feb6e406cc109d751bdf425b3e31a743736a93dd983221a51` |

M2 broader hardware/second-PC validation and actual user-observed UAC rejection
remain open. M6 is unchanged.

References: [CPython Windows stdio initialization](https://github.com/python/cpython/blob/v3.12.10/Python/initconfig.c),
[M3 history](./M3_VALIDATION.md), [M4 history](./M4_VALIDATION.md), [M5 history](./M5_VALIDATION.md).