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

Local M5 implementation and the checks above are complete. The user made the
repository public, confirmed through GitHub's repository API. CLI authentication
now succeeds as maaz0166 with repository/workflow access. Source commit and
GitHub-hosted release execution are the next publication steps.

The manual Native release workflow must build and verify its assets before
publishing the preview. The local publisher is also available:

```powershell
./packaging/publish_release.ps1
```

Verify the existing remote release/assets after publication, including all
asset hashes; do not repeat creation if an outcome is uncertain. The publisher
stops on a failed/ambiguous creation instead of retrying or replacing assets.
No GitHub-hosted build execution is claimed until its actual run is observed.
The winget manifest is not submitted to Microsoft's catalog by this work.

References: [Inno Setup](https://jrsoftware.org/isinfo.php),
[environment notifications](https://jrsoftware.org/ishelp/topic_setup_changesenvironment.htm),
[winget manifests](https://learn.microsoft.com/en-us/windows/package-manager/package/manifest).
