# QueryForge

QueryForge helps C++ developers choose how to query large in-memory datasets by benchmarking scans and indexes against representative data and query patterns.

If you are deciding whether to use a linear scan, hash index, sorted index, or a mix of strategies, QueryForge gives you measured latency distributions, memory overhead, build cost, and a plain-English recommendation from the terminal.

## Quickstart

Build from source:

```powershell
cmake -S . -B build
cmake --build build
```

Run against the included CSV example:

```powershell
.\build\Debug\queryforge.exe run --input examples\trades.csv --where symbol=AAPL --strategy all --runs 30
```

Run against generated data:

```powershell
.\build\Debug\queryforge.exe run --rows 1000000 --where symbol=AAPL --strategy all --warmup 5 --runs 30 --seed 42
```

Example output:

```text
Result:
Strategy      build     p50       p95       p99       avg       memory      matches
-----------------------------------------------------------------------------------
linear_scan   0.0 ms    23.4 ms   26.8 ms   30.1 ms   24.0 ms   0 B         1,955
hash_index    7.2 ms    0.1 ms    0.1 ms    0.2 ms    0.1 ms    8.1 MB      1,955

Recommendation:
  Use hash_index for this workload. Estimated total cost for 1 repeated queries is 7.3 ms.
```

## Supported Workloads

Current data model:

```csv
symbol,timestamp,price,quantity
AAPL,1600000000000000000,185.12,100
```

Current query filters:

- `symbol=AAPL`
- `timestamp>=1600000000000000000`
- `price<250`
- `quantity>100`

Current strategies:

- `linear_scan`
- `hash_index` for symbol equality queries
- `sorted_index` for numeric range queries
- `all` to run every strategy that applies

## CLI Examples

Text output:

```powershell
.\build\Debug\queryforge.exe run --input examples\trades.csv --where symbol=AAPL --strategy all
```

JSON output:

```powershell
.\build\Debug\queryforge.exe run --input examples\trades.csv --where symbol=AAPL --strategy all --output json
```

Repeated-query model:

```powershell
.\build\Debug\queryforge.exe run --rows 1000000 --where symbol=AAPL --strategy all --repeat-count 100
```

## Prerequisites

- CMake 3.20 or newer
- A C++20 compiler (MSVC 2019+, GCC 10+, or Clang 12+)

On Windows with the Visual Studio generator, the executable is typically at `build\Debug\queryforge.exe` or `build\Release\queryforge.exe`. With Ninja or other single-config generators, it is at `build\queryforge.exe`.

If CMake cannot find a compiler, open a Developer PowerShell for Visual Studio or pass an explicit generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
```

## Tests

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Or:

```powershell
cmake --build build --target test
```

## Roadmap

- Load broader file formats and custom schemas.
- Add more query strategies and memory layouts.
- Improve recommendation logic with richer workload modeling.
- Publish binaries through GitHub Releases.

## Project Layout

```text
QueryForge/
├── CMakeLists.txt
├── include/queryforge/   # public headers
├── src/                  # implementations
├── tests/                # unit tests (Catch2)
├── examples/             # sample datasets
└── .github/              # CI and issue templates
```
