# hwspec

A command-line tool that shows your PC's hardware specs (CPU, memory, OS, and more later) in one command. Windows first.

**Status: milestone M1.** `--cpu`, `--mem`, `--os` and `--all` work. More sections, `--json` and detail views come in M2.

## Usage

```
hwspec               # every section
hwspec --cpu         # just the CPU
hwspec --cpu --mem   # combine sections, in any order
hwspec --help        # all options
hwspec --version
```

## Build

You need CMake and a C++ compiler for Windows. Use whichever setup you have.

**MinGW (GCC) with Ninja** (the program lands directly in `build`):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
build\hwspec.exe
```

**MSVC / Visual Studio** (the program lands in `build\Release`):

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build\Release\hwspec.exe
```

GitHub Actions builds with MSVC on every push.

## Layout

```
core/include/hwspec/   public headers: data types, collectors, formatting, sections
core/src/              formatting, report rendering, the section list
core/src/windows/      the Windows collectors (registry and Win32 calls), one file per section
core/src/stub/         empty collectors for non-Windows builds (development and tests only)
cli/                   argument parsing, run(), and a thin main()
tests/                 tests, run with ctest
python/                Python package (name reserved on PyPI; pybind11 binding comes in M4)
```

## How a section works

A collector (`collect_cpu`) reads the machine and returns plain data. A formatter (`format_cpu`)
turns that data into a titled list of fields, with `Unknown` for anything that could not be read.
`default_sections()` lists the sections; the CLI builds its `--flags` from that list.
