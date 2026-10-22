# M5 native release and distribution

The user explicitly requested implementing M5 and publishing on GitHub after
Python 0.4.1. Native release version 0.5.0 is prepared as a preview, retaining
the outstanding M2 second-PC and M3 observed-UAC-rejection acceptance items.
The separately published Python version remains 0.4.1; its immutable registry
files have not been replaced, and Python 0.5.0 has not been published.

## Deliverables

- MSVC x64 standalone executable with a static C++ runtime and Windows version
  resource. The static-runtime option defaults OFF for ordinary/Python builds;
  Python bindings keep their existing runtime configuration.
- Portable ZIP with exactly `hwspec.exe`, `LICENSE` and `README.txt`.
- Per-user Inno Setup installer: Windows 10/11 x64 only, no requested elevation,
  default `%LOCALAPPDATA%\Programs\hwspec`, optional user PATH task and a Start
  menu shortcut with `--pause`. Installer state uses a stable application GUID.
- A generated three-file winget manifest with the exact setup SHA-256 and the
  versioned GitHub download URL, plus a manifest ZIP and asset checksums.
- Real native console screenshots in the root README. They show CPU/memory/OS
  and CPU detail, avoiding hostname, MAC address and serial-number output.
- A manual GitHub Actions release workflow builds/tests/packages and uploads CI
  artifacts. Its explicit `publish` input defaults false. When selected, it
  creates a preview draft, uploads only the checked assets, and publishes it.
  Existing releases/assets are never overwritten. Normal pushes do not publish.

## Local verification: 2026-10-22

Windows 11 Pro x64, Dell Latitude 5500, MSVC 19.44.35217.0, Windows SDK
10.0.26100.0, installed Inno Setup 6 and winget 1.29.380.

```powershell
./packaging/build_release.ps1
./tests/verify_installer.ps1 -Installer ./dist/native/hwspec-0.5.0-windows-x64-setup.exe -WorkDirectory ./build/installer-validation-final
./tests/verify_portable.ps1 -Archive ./dist/native/hwspec-0.5.0-windows-x64.zip
```

- Native Release build and all seven CTests passed with the static runtime.
- Six distribution smoke checks passed against the standalone executable,
  installed executable and extracted portable ZIP. Child PATH contained only
  Windows directories, with Python/venv/developer directories removed. Checks
  cover version/resource, all-section JSON, details, missing-data/skip policy,
  noninteractive pause, combined text selection and invalid-option exit status.
- `dumpbin /DEPENDENTS` lists only Windows system DLLs. There are no MSVCP,
  VCRUNTIME, UCRTBASE or Python imports. No developer runtime DLL is shipped in
  the standalone ZIP or installer. A separate clean physical machine without
  development tools has not been exercised; this is local dependency/isolation
  evidence, not a claim of second-PC acceptance.
- The final installer lifecycle run exited 0 with **28 checks passed**. It
  installed under a workspace path containing spaces and Unicode, ran without
  Python, reinstalled without duplicating PATH, preserved later user PATH edits,
  preserved a quoted pre-existing PATH entry, honored the disabled PATH task,
  handled an initially absent PATH value, removed its executable/uninstall
  registration, and restored the test user's original raw PATH afterward.
- The first harness run passed its 27 functional checks but attempted to rerun
  the already-completed uninstaller during cleanup. The guard now checks product
  registration/executable rather than a transient leftover uninstall stub;
  the full 28-check rerun, including restoration, passed. It is not recorded as
  a successful first run.
- The native winget client reported `Manifest validation succeeded`. Catalog
  submission/acceptance and public download accessibility are separate checks.
- Console screenshots were visually inspected. The agent's isolated desktop
  did not expose console handles; an interactive-desktop helper captured the
  real windows. Only the helper's screenshot processes were closed afterward.

Local evidence is under `build/release/`, `build/installer-validation-final/`
and `build/portable-validation/`. Installer logs remain local; they contain
the test account's paths and are not release assets.

## Prepared asset inventory

| Asset | SHA-256 |
| --- | --- |
| `hwspec-0.5.0-windows-x64.exe` | `79280e3881a3dec5d4455e96c928b3465a350b9d16afe74c423d890c43ae8184` |
| `hwspec-0.5.0-windows-x64.zip` | `b73e94a05e39faba2bf6954eeb35b48b0a34e3735248b89fb58b66e733ded7e5` |
| `hwspec-0.5.0-windows-x64-setup.exe` | `a5ae582a65e57e06b50e1eb245928d34f972ba150d8439321b3599217fbf81ce` |
| `hwspec-0.5.0-winget.zip` | `da6dd8d8962d9fe74640260a44085fed95c1a8e3318a509d9687789bc90aea7e` |

These hashes identify this tested local build. A subsequent CI build can have
different timestamps/installer hashes; the builder generates a matching winget
manifest and checksums for that build. Do not mix manifests between builds.

## Publication status and remaining checks

Native M5 preview [v0.5.0](https://github.com/maaz0166/hwspec/releases/tag/v0.5.0)
is publicly published from source commit `900613912c472c30f3cb175075452ffab868316b`.
The user changed repository visibility; GitHub's API confirmed it is public.
The [release workflow](https://github.com/maaz0166/hwspec/actions/runs/39822691058)
completed successfully, including package and publish jobs:

- All seven static-runtime CTests passed.
- Six distribution checks passed against each standalone, portable and installed executable.
- All 28 installer lifecycle checks passed; winget manifest validation passed.
- Five assets were uploaded, the draft published, and the release verified as
  public, non-draft and prerelease. Existing releases were not overwritten.

Earlier hosted attempts exposed three issues that were corrected before this
successful run: WinGet provisioning needed `-AllUsers`; the installer test
needed the registered uninstall executable rather than a fixed `unins000.exe`
name; and workflow installation tokens cannot rely on a user-role permission
field. The publisher now checks repository access and lets release creation
itself enforce write permission. Failed runs are not counted as successes.
The revised installer harness also passed all 28 checks locally before pushing.

## Published asset verification: 2026-10-22

All five assets were downloaded through public URLs without authentication.
Their hashes match GitHub's asset digests, and all four package hashes match
the published `SHA256SUMS.txt`. The downloaded winget manifest references the
exact published installer hash and passes native `winget validate`.
The downloaded standalone executable and extracted portable archive each passed
all six distribution checks on the reference laptop with Python/toolchain PATH
entries removed. Evidence is under `build/m5-public-download/`,
`build/m5-public-standalone/` and `build/m5-public-portable/`.

| Published asset | SHA-256 |
| --- | --- |
| `hwspec-0.5.0-windows-x64.exe` | `3d3e1d7c7bc9fd64b4aff967a0c34fbb0e8cc35e7b3019e636f52e2b4949d699` |
| `hwspec-0.5.0-windows-x64.zip` | `315a2e72dd197ec622f31af1ec5b91f98675196528a5f6a56bca8b4a7416ec3d` |
| `hwspec-0.5.0-windows-x64-setup.exe` | `20427338a89e955ad946a376f8885434a4c3de6f051c6a12f80d2ca9643fbb04` |
| `hwspec-0.5.0-winget.zip` | `f9663eec4380fe5d07e2a97b3e6d651c45440d5ece6e312c0b66640a5ec5fc43` |
These are CI-built release assets. The earlier local inventory above remains
preserved for comparison and must not be mixed with published manifests.
`SHA256SUMS.txt` itself has SHA-256
`d5fc19797d2dd91745145a0410f4a06f16fc8cf8f403047c27975d488d4f6d8b`.

M2 second-PC/broader hardware coverage and M3 actual user-observed UAC rejection
remain open. Hosted Windows Server runner checks do not establish Windows 10
or second physical PC acceptance. The winget manifest has not been submitted to
Microsoft's catalog. Python registry version 0.4.1 is unchanged; this native
release did not publish Python 0.5.0. M6 remains deferred.

References: [Inno Setup](https://jrsoftware.org/isinfo.php),
[environment notifications](https://jrsoftware.org/ishelp/topic_setup_changesenvironment.htm),
[winget manifests](https://learn.microsoft.com/en-us/windows/package-manager/package/manifest).
