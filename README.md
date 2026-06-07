# QueryForge

QueryForge helps C++ developers find the fastest way to query large in-memory datasets by benchmarking scans, indexes, and memory layouts from the terminal.

## Prerequisites

- CMake 3.20 or newer
- A C++20 compiler (MSVC 2019+, GCC 10+, or Clang 12+)

## Build

```powershell
cmake -S . -B build
cmake --build build
```

On Windows with the Visual Studio generator, the executable is typically at `build\Debug\queryforge.exe` (or `build\Release\queryforge.exe`). With Ninja or other single-config generators, it is at `build\queryforge.exe`.

If CMake cannot find a compiler, open a **Developer PowerShell for VS** or pass an explicit generator, for example:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
```

## Usage

```powershell
.\build\queryforge.exe --help
.\build\Debug\queryforge.exe run --rows 1000000 --symbol AAPL
```

The `run` command generates a synthetic trade dataset and benchmarks a linear scan for the requested symbol.

## Tests

```powershell
ctest --test-dir build --output-on-failure
```

With the Visual Studio generator, pass the configuration:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Or:

```powershell
cmake --build build --target test
```

## Project layout

```
QueryForge/
├── CMakeLists.txt
├── include/queryforge/   # public headers
│   ├── core/             # domain types (TradeEvent)
│   ├── data/             # dataset generation
│   ├── query/            # query strategies and benchmarks
│   ├── util/             # formatting helpers
│   └── cli/              # command handlers
├── src/                  # implementations (mirrors include/)
├── tests/                # unit tests (Catch2)
└── examples/             # example datasets and configs (future phases)
```
