# hwspec

A command-line tool that shows your PC's hardware specs (CPU, memory, GPU, disk, battery) in one command. Windows first.

**Status: milestone M0.** The project builds and CI runs; the real features arrive from M1.

## Build (Windows, MSVC)

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build\Release\hwspec.exe --version
```

## Layout

```
core/      C++ core library (collectors live under core/src/windows/ later)
cli/       command-line program
tests/     tests, run with ctest
python/    Python package (name reserved on PyPI; pybind11 binding comes in M4)
```
