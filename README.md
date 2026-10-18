# hwspec

A command-line tool that shows your PC's hardware specs (CPU, memory, GPU, disk, battery) in one command. Windows first.

**Status: milestone M0.** The project builds and CI runs; the real features arrive from M1.

## Build

You need CMake and a C++ compiler for Windows. Use whichever setup you have.

**MinGW (GCC) with Ninja** (single-configuration generator: the program lands directly in `build`):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
build\hwspec.exe --version
```

**MSVC / Visual Studio** (multi-configuration generator: the program lands in `build\Release`):

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build\Release\hwspec.exe --version
```

Either way, the last command should print `hwspec 0.0.1`. GitHub Actions builds with MSVC on every push.

## Layout

```
core/      C++ core library (collectors live under core/src/windows/ later)
cli/       command-line program
tests/     tests, run with ctest
python/    Python package (name reserved on PyPI; pybind11 binding comes in M4)
```
