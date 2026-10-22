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

Pending: push reviewed source, run GitHub native release and Python-wheel
workflows, validate repaired wheels/sdist, publish TestPyPI then verify a fresh
install, publish the identical files to PyPI then verify another fresh install.
The Python workflow builds all six wheels from one frozen sdist and supplies a
native executable so both parity tests run rather than skip.

GitHub publication must include native EXE/ZIP/installer, winget manifest and
checksums; public downloads must match. Installer lifecycle checks must pass.
Winget catalog submission is not implied by publishing the manifest.

M2 broader hardware/second-PC validation and actual user-observed UAC rejection
remain open. M6 is unchanged.

References: [CPython Windows stdio initialization](https://github.com/python/cpython/blob/v3.12.10/Python/initconfig.c),
[M3 history](./M3_VALIDATION.md), [M4 history](./M4_VALIDATION.md), [M5 history](./M5_VALIDATION.md).