# M4 Python package

M4 source is implemented as v0.4.0. The user requested proceeding with M4 on
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
twine 7.0.0, cibuildwheel 4.3.3. No package was published.

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

TestPyPI upload, clean-environment install from TestPyPI, PyPI upload and install
from PyPI are still pending. These require explicit publication authorization
and access to the package's publishing account. Successful local wheel tests do
not claim that a public registry release exists.

No local `.pypirc`, publishing environment variables or matching keyring
credentials were available during this session. Public registry metadata still
listed 0.0.1, with no 0.4.0 release; account ownership has not been verified.
Do not paste API tokens into chat or put them in commands/history. Configure
credentials locally for each registry or set up approved trusted publishing.

After publication is authorized and credentials are configured, upload only the
repaired wheels and source archive:

```powershell
./build/python-build-env/Scripts/python.exe -m twine upload --repository testpypi dist/cibuildwheel/*.whl dist/hwspec-0.4.0.tar.gz
python -m venv build/testpypi-install
./build/testpypi-install/Scripts/python.exe -m pip install --index-url https://test.pypi.org/simple/ --only-binary=:all: --no-deps hwspec==0.4.0
./build/testpypi-install/Scripts/hwspec.exe --cpu --json
./build/testpypi-install/Scripts/python.exe -m hwspec --all --json
./build/testpypi-install/Scripts/python.exe -c "import hwspec; print(hwspec.collect('cpu'))"
```

Run `tests/test_python.py` with that installed interpreter from outside the
checkout, as described above. Only after TestPyPI installation/tests pass and
production publication is approved, upload the same bytes to PyPI:

```powershell
./build/python-build-env/Scripts/python.exe -m twine upload dist/cibuildwheel/*.whl dist/hwspec-0.4.0.tar.gz
python -m venv build/pypi-install
./build/pypi-install/Scripts/python.exe -m pip install --index-url https://pypi.org/simple/ --only-binary=:all: --no-deps hwspec==0.4.0
./build/pypi-install/Scripts/hwspec.exe --cpu --json
./build/pypi-install/Scripts/python.exe -m hwspec --all --json
```

Repeat the installed-package suite against the PyPI installation. These commands
are a runbook, not evidence of uploads. Do not automatically publish on a normal
Git push or pull request.

## Remaining acceptance work

- All six local wheel builds/tests passed; the GitHub-hosted workflow has not
  yet been observed running against this change.
- Python UAC, preserved virtual environment/arguments, pause and Enter-to-close
  passed the live console integration check above.
- Second-PC, Task Manager, GPU-source discrepancies and user-observed native
  UAC/close-on-Enter checks remain tracked in M2/M3 validation.
- M5 installers, releases and other distribution channels have not started.

## Build-system references

- [pybind11 CMake and Python packaging](https://pybind11.readthedocs.io/en/stable/compiling.html)
- [scikit-build-core configuration](https://scikit-build-core.readthedocs.io/en/stable/configuration/)
- [cibuildwheel](https://cibuildwheel.pypa.io/en/stable/)
