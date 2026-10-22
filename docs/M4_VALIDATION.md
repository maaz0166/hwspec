# M4 Python package

M4 is delivered and published, initially as v0.4.0 and now v0.4.1 with the cache
and configured-RAM-speed fixes. The user requested proceeding with M4 on
2026-10-22 while the externally dependent M2/M3 checks remain documented.
Those milestones have not been silently marked fully verified.

## Implementation

- The pybind11 extension links the same core and CLI libraries as the executable.
- `hwspec.collect(sections=None, *, detail=False, skip_admin=False)` returns the
  CLI JSON schema as Python dictionaries/lists and typed values, with None for
  missing values. Names are validated before collection. Library calls are
  noninteractive and release the GIL during collection.
- The installed `hwspec` command and `python -m hwspec` call the existing C++
  `run()` function with stdout/stderr redirected to Python's UTF-8 streams.
- Python elevation targets `sys.executable -m hwspec`, preserving its virtual
  environment. Native standalone elevation continues to use the executable's
  own path. Both routes share argument quoting, removal of --elevate, and pause
  insertion. C++ tests cover the Python prefix and quote-sensitive parameters.
- The extension is opt-in for ordinary CMake builds (`HWSPEC_BUILD_PYTHON=ON`)
  and rejects non-Windows/non-x64 targets. Portable native tests keep working.

**Layout adjustment from the plan:** packaging metadata moved from the old
`python/pyproject.toml` placeholder to root `pyproject.toml`. This makes the
source archive self-contained: CMake, core, CLI, Python sources and tests are
inside it rather than referenced through a parent directory. Binding sources
and the Python package remain under `python/`.

## Local verification: 2026-10-22

Windows 11 Pro x64, Dell Latitude 5500, Python 3.12.10, MSVC 19.44.35217.0.
Build environment tools: pybind11 3.1.0, scikit-build-core 1.1.1, build 1.6.1,
twine 7.0.0, cibuildwheel 4.3.3. The initial local checks below preceded the
approved publication recorded later in this report.

```powershell
python -m venv build/python-build-env
./build/python-build-env/Scripts/python.exe -m pip install build twine scikit-build-core pybind11 cibuildwheel
$env:CMAKE_GENERATOR = 'Visual Studio 17 2022'
$env:CMAKE_GENERATOR_PLATFORM = 'x64'
./build/python-build-env/Scripts/python.exe -m build --no-isolation --outdir dist

python -m venv build/python-wheel-test-env
./build/python-wheel-test-env/Scripts/python.exe -m pip install --no-index ./dist/hwspec-0.4.0-cp312-cp312-win_amd64.whl
./build/python-wheel-test-env/Scripts/python.exe -m pip check
./build/python-build-env/Scripts/python.exe -m twine check ./dist/hwspec-0.4.0.tar.gz ./dist/hwspec-0.4.0-cp312-cp312-win_amd64.whl
```

- `build` successfully made the source distribution and then built a Windows
  wheel from that archive, outside the Git checkout. Initial build tools were
  preinstalled in the dedicated build environment (--no-isolation).
- The resulting wheel installed offline into a fresh virtual environment with
  no build dependencies. `pip check` and both metadata checks passed.
- All 15 installed-package tests passed from a temporary working directory
  outside the source tree, with `HWSPEC_NATIVE_EXE` set to the v0.4.0 standalone
  executable. The tests cover API selection, typed values, None/health handling,
  invalid inputs, concurrency, no library output, pip/module CLI behavior,
  piped JSON/pause, exit codes, version metadata and static-value parity.
- All seven CTest executables and the JSON smoke script passed independently
  with MSVC and MinGW. The tested MSVC native executable is under
  `build/msvc/verified/` because an earlier live elevation console held the
  original Release executable open.
- Source archive inspection confirmed the Windows core, CLI and Python test
  sources are included. The initial unrepaired wheel contains the Python package,
  extension, license and distribution metadata; no raw hardware captures are packaged.
- `cibuildwheel --print-build-identifiers` selected precisely CPython 3.9,
  3.10, 3.11, 3.12, 3.13 and 3.14 on Windows AMD64.

### cibuildwheel distribution check

```powershell
$env:CIBW_BUILD = 'cp312-win_amd64'
$env:CIBW_CACHE_PATH = 'C:/path/to/repo/build/cibuildwheel-cache'
./build/python-build-env/Scripts/python.exe -m cibuildwheel --platform windows --output-dir dist/cibuildwheel
```

The actual cibuildwheel 4.3.3 CPython 3.12 build passed locally. It created an
isolated build environment, built with MSVC, repaired DLL dependencies using
delvewheel 1.13.2, installed the repaired wheel in a fresh test environment and
ran the Python suite: 14 passed, with the optional standalone comparison skipped
in that context. The original separate local test ran all 15 including parity.

The repair step bundled `msvcp140.dll` under a mangled package-local name and
patched the DLL loader. Distribute the repaired wheel from `dist/cibuildwheel/`;
the unrepaired `python -m build` wheel can otherwise rely on the developer's
installed Microsoft C++ runtime. The repaired artifact's SHA-256 is
`d7722911036a9c6fb634d920fd775b050736ef4671570fd866c769b4c4b96549`.

The repaired wheel was then installed offline into the separate local clean
test environment, replacing the raw wheel. All 15 tests passed again, including
standalone parity. Its `twine check` and `pip check` also passed. The final
source distribution was refreshed after documentation updates and passed
metadata validation.

For the installed-package test, run the following with an absolute interpreter
path from a directory outside the repository (the test file may remain in it):

```powershell
$env:HWSPEC_NATIVE_EXE = 'C:/path/to/repo/build/msvc/verified/hwspec.exe'
& 'C:/path/to/repo/build/python-wheel-test-env/Scripts/python.exe' 'C:/path/to/repo/tests/test_python.py'
```

### Installed-package UAC and console check

The repaired CPython 3.12 wheel also passed a live standard-user to elevated
virtual-environment test on 2026-10-22:

```powershell
./tests/verify_python_uac.ps1 -Python ./build/python-wheel-test-env/Scripts/python.exe
```

Approve both UAC dialogs (the observer and the application). The observer checks
the exact installed interpreter and application arguments before attaching to
that process's console. It reads the console and submits Enter through that
console's input buffer; it does not send desktop-wide keys.

The parent returned 0 with empty stdout/stderr. The child used the installed
virtual-environment interpreter with `-m hwspec --cpu --pause`, had an elevated
token, displayed only CPU output and `Press Enter to close...`, and exited 0
after Enter. The test saved the command line and structured results under
`build/python-uac-validation/20261022-110840-883/`. This verifies the Python
elevation/console path; it does not establish that a person rejected the
separate native application's UAC prompt.

### Complete local interpreter matrix

The remaining five cibuildwheel builds completed successfully on this Windows
host. Each built with MSVC, repaired dependencies, installed the repaired wheel
in a fresh environment and passed all 15 tests, including standalone parity.
Together with the separate CPython 3.12 run, all six supported interpreters have
passed installed-package validation:

| CPython | Wheel | Installed tests |
| --- | --- | --- |
| 3.9.13 | cp39-win_amd64 | 15 passed |
| 3.10.11 | cp310-win_amd64 | 15 passed |
| 3.11.9 | cp311-win_amd64 | 15 passed |
| 3.12.10 | cp312-win_amd64 | 15 passed after repaired-wheel installation |
| 3.13.16 | cp313-win_amd64 | 15 passed |
| 3.14.8 | cp314-win_amd64 | 15 passed |

```powershell
$env:CIBW_BUILD = 'cp39-win_amd64 cp310-win_amd64 cp311-win_amd64 cp313-win_amd64 cp314-win_amd64'
$env:CIBW_CACHE_PATH = 'C:/path/to/repo/build/cibuildwheel-cache'
$env:HWSPEC_NATIVE_EXE = 'C:/path/to/repo/build/msvc/verified/hwspec.exe'
./build/python-build-env/Scripts/python.exe -m cibuildwheel --platform windows --output-dir dist/cibuildwheel
```

The five-build command completed with exit 0; its local log is
`build/m4-wheel-matrix.log`. These are real local matrix results, not a claim
that a GitHub Actions run has completed. The distributable bundle is
`dist/hwspec-0.4.0-windows-x64-python.zip`; it contains the six repaired wheels,
source archive, SHA-256 manifest, offline installation instructions and this
report. The unrepaired development wheel is excluded.

## CI and publication

[Python wheels workflow](../.github/workflows/python-wheels.yml) builds and tests
the six CPython versions on Windows x64 using cibuildwheel (including dependency
repair), builds an sdist,
checks metadata, and uploads downloadable CI artifacts. It does not publish
packages. All six interpreter versions passed the local build/test path above;
execution on a GitHub-hosted runner has not been observed in this session.

### Approved publication and registry installation: 2026-10-22

The user saved separate registry credentials in the local keyring and explicitly
authorized TestPyPI publication, verification, then PyPI publication. Credentials
were used through Twine's keyring integration and were never displayed.

- [TestPyPI 0.4.0](https://test.pypi.org/project/hwspec/0.4.0/) accepted all six
  repaired wheels and the source archive. A fresh CPython 3.12 environment,
  `build/testpypi-install/`, installed from TestPyPI with caching disabled and
  no dependencies or source builds. All 15 installed-package tests passed from
  outside the checkout, including standalone parity; `pip check` passed.
- Only after that verification, the identical seven files were uploaded to
  [PyPI 0.4.0](https://pypi.org/project/hwspec/0.4.0/). A separate fresh environment,
  `build/pypi-install/`, installed from PyPI and passed the same 15 tests and
  dependency check. The suite exercises the installed `hwspec` command, module
  command, Python API, JSON, missing-data handling and invalid-input behavior.
- Each registry's version-specific JSON metadata lists exactly the seven expected
  files. All seven SHA-256 hashes on each registry match the tested local manifest.
  No raw development wheel or native validation capture was uploaded.
- Published source SHA-256:
  `c8214a755e253d4193c0d70b103f8acca54eb64992358f17b06a27a93da7ca11`.
  The published CPython 3.12 wheel retains the SHA-256 recorded above.
- Local evidence includes `build/testpypi-published-files.json`,
  `build/pypi-published-files.json`, and each registry's pip installation report
  under `build/`. These reports contain public package metadata, not credentials.

The offline bundle and source archive retain their pre-publication documentation
snapshot. Their release bytes were preserved; this report records the later
successful registry publication. GitHub-hosted workflow execution remains
unobserved and is not inferred from these local results.

Commands used for the TestPyPI stage (the upload also used `--non-interactive`
and `--disable-progress-bar`; installation also used pip `--isolated`,
`--no-cache-dir`, `--report` and `--disable-pip-version-check`):

```powershell
./build/python-build-env/Scripts/python.exe -m twine upload --repository testpypi dist/cibuildwheel/*.whl dist/hwspec-0.4.0.tar.gz
python -m venv build/testpypi-install
./build/testpypi-install/Scripts/python.exe -m pip install --index-url https://test.pypi.org/simple/ --only-binary=:all: --no-deps hwspec==0.4.0
./build/testpypi-install/Scripts/hwspec.exe --cpu --json
./build/testpypi-install/Scripts/python.exe -m hwspec --all --json
./build/testpypi-install/Scripts/python.exe -c "import hwspec; print(hwspec.collect('cpu'))"
```

After the TestPyPI suite and dependency check passed, the same files were
published to PyPI and installed with the equivalent isolated/no-cache options:

```powershell
./build/python-build-env/Scripts/python.exe -m twine upload dist/cibuildwheel/*.whl dist/hwspec-0.4.0.tar.gz
python -m venv build/pypi-install
./build/pypi-install/Scripts/python.exe -m pip install --index-url https://pypi.org/simple/ --only-binary=:all: --no-deps hwspec==0.4.0
./build/pypi-install/Scripts/hwspec.exe --cpu --json
./build/pypi-install/Scripts/python.exe -m hwspec --all --json
```

Both installations were tested using `tests/test_python.py` with
`HWSPEC_NATIVE_EXE` set to the verified native executable, from outside the
checkout. Each ran 15 tests with no failures or skips. Do not repeat these
uploads for the existing version; publishing future releases requires approval.
Normal Git pushes and pull requests do not publish packages.

## v0.4.1 publication follow-up: 2026-10-22

The user explicitly authorized uploading the latest corrections to TestPyPI and
PyPI. Version 0.4.1 includes the M2 cache-total and configured-RAM-speed fixes;
the published 0.4.0 files were preserved. CMake and root package versions both
changed to 0.4.1. The Python suite now has 16 tests, adding native-versus-Python
parity for detailed cache totals and memory modules, including configured speed.

All six Windows AMD64 wheels (CPython 3.9.13, 3.10.11, 3.11.9, 3.12.10, 3.13.16
and 3.14.8) were built from the same frozen source archive with cibuildwheel
4.3.3. Each repaired wheel passed all 16 installed-package tests, with neither
failures nor skips. The new native MSVC build passed all seven CTests and JSON
smoke checks. Earlier native MinGW correctness checks remain recorded in M2.

```powershell
./build/python-build-env/Scripts/python.exe -m build --sdist --no-isolation --outdir dist/0.4.1
$env:CIBW_BUILD = 'cp39-win_amd64 cp310-win_amd64 cp311-win_amd64 cp312-win_amd64 cp313-win_amd64 cp314-win_amd64'
$env:CIBW_CACHE_PATH = 'C:/path/to/repo/build/cibuildwheel-cache'
$env:CMAKE_BUILD_PARALLEL_LEVEL = '4'
$env:HWSPEC_NATIVE_EXE = 'C:/path/to/repo/build/msvc/acceptance/hwspec.exe'
./build/python-build-env/Scripts/python.exe -m cibuildwheel dist/0.4.1/hwspec-0.4.1.tar.gz --platform windows --output-dir dist/0.4.1/wheels
./build/python-build-env/Scripts/python.exe -m twine check dist/0.4.1/wheels/*.whl dist/0.4.1/hwspec-0.4.1.tar.gz
```

The interrupted initial process produced no wheels or uploads. The resumed
six-version command exited 0; its log is `build/v041-wheels.log`. Archive audits
verified that the source contains both corrections and the new test, and each
wheel has matching version/tag metadata, its extension, console entry point and
package-local repaired C++ runtime dependency. All seven metadata checks passed.

Publication proceeded in the authorized order:

1. [TestPyPI 0.4.1](https://test.pypi.org/project/hwspec/0.4.1/) accepted the six
   repaired wheels and sdist. All seven registry hashes matched the frozen
   local manifest. A fresh CPython 3.12 environment, `build/v041-testpypi-install`,
   installed from TestPyPI without cache, dependencies or source compilation.
   All 16 tests passed from outside the checkout with standalone parity enabled;
   `pip check` passed. Explicit real-device assertions confirmed L1 256 KiB,
   L2 1 MiB, L3 6 MiB and configured RAM speed 2400, matching the UI evidence.
2. Only then were the identical seven files uploaded to
   [PyPI 0.4.1](https://pypi.org/project/hwspec/0.4.1/). A separate fresh environment,
   `build/v041-pypi-install`, installed using the normal PyPI package index and
   passed the same 16 tests, dependency check and real-device assertions. The
   first installation lookup preceded the simple-index update; after the index
   advertised 0.4.1, retry succeeded. No duplicate upload was attempted.

Both installations used pip `--isolated`, `--no-cache-dir`, `--only-binary=:all:`,
`--no-deps`, an explicit registry index URL, and installation reports under
`build/v041-*-install-report.json`. All seven public file names and SHA-256
hashes on **each** registry match `dist/0.4.1/manifest.json`. Published source:
`01c10e18c5f2c73e37b2abaaa06214f943ff84f0917522c103eddf84def9b6f2`.
Published CPython 3.12 wheel:
`ef01b5f4569e277f71333740e85ed641fbccc1d01ca60e8f4cee1765859b6a8d`.
Registry metadata captures are `build/v041-testpypi-published-files.json` and
`build/v041-pypi-published-files.json`. Credentials stayed in the local keyring.

```powershell
python -m pip install --upgrade hwspec==0.4.1
hwspec --cpu --mem -d --json
```

These are local build and real registry-installation results. No GitHub-hosted
run or additional physical PC is inferred from them. The second-PC and observed
UAC-rejection acceptance items remain open.

## Remaining acceptance work

- M2 Task Manager corrections postdate 0.4.0 and are included in published
  0.4.1, as recorded above. Historical 0.4.0 results and hashes remain unchanged.
- All six local wheel builds/tests passed; the GitHub-hosted workflow has not
  yet been observed running against this change.
- Python UAC, preserved virtual environment/arguments, pause and Enter-to-close
  passed the live console integration check above.
- Second-PC and actual UAC rejection remain pending in M2/M3 validation.
  Task Manager comparison and native pause/Enter-to-close have since passed;
  GPU-source distinctions remain documented without inventing matching values.
- M5 installers, releases and other distribution channels have not started.

## Build-system references

- [pybind11 CMake and Python packaging](https://pybind11.readthedocs.io/en/stable/compiling.html)
- [scikit-build-core configuration](https://scikit-build-core.readthedocs.io/en/stable/configuration/)
- [cibuildwheel](https://cibuildwheel.pypa.io/en/stable/)
