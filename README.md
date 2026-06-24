# QueryForge

[![CI](https://github.com/your-org/QueryForge/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/QueryForge/actions/workflows/ci.yml)

QueryForge helps C++ developers choose how to query large in-memory datasets by benchmarking scans and indexes against representative data and query patterns.

If you are deciding whether to use a linear scan, hash index, sorted index, or a mix of strategies, QueryForge gives you measured latency distributions, memory overhead, build cost, and a plain-English recommendation from the terminal.

## Quickstart

Build from source:

```powershell
cmake -S . -B build
cmake --build build
```

Install locally:

```powershell
cmake --install build --prefix install
```

Run against the included CSV example:

```powershell
.\build\Debug\queryforge.exe run --input examples\trades.csv --where symbol=AAPL --strategy all --runs 30
```

Inspect a CSV schema:

```powershell
.\build\Debug\queryforge.exe inspect examples\users.csv
```

Run against an arbitrary CSV schema with inferred types:

```powershell
.\build\Debug\queryforge.exe run --input examples\users.csv --where country=US --where age>=30 --strategy all
```

Run with an explicit schema file:

```powershell
.\build\Debug\queryforge.exe run --input examples\users.csv --schema-file examples\schemas\users.json --where country=US
```

Run against generated data:

```powershell
.\build\Debug\queryforge.exe run --rows 1000000 --where symbol=AAPL --strategy all --warmup 5 --runs 30 --seed 42
```

Generate arbitrary synthetic schemas:

```powershell
.\build\Debug\queryforge.exe run --rows 100000 --schema user_id:int64,country:string,score:double --where country=US --strategy all
```

Example output:

```text
Result:
Strategy      build     p50       p95       p99       avg       stddev    memory      matches
-----------------------------------------------------------------------------------------------
linear_scan   0.0 ms    23.4 ms   26.8 ms   30.1 ms   24.0 ms   1.2 ms    0 B         1,955
hash_index    7.2 ms    0.1 ms    0.1 ms    0.2 ms    0.1 ms    0.0 ms    8.1 MB      1,955

Recommendation:
  Use hash_index for this workload. Estimated total cost for 1 repeated queries is 7.3 ms.
```

## Supported Workloads

CSV files can use any headered schema. QueryForge infers column types by default, or you can pass `--schema` / `--schema-file`.

Quoted CSV fields are supported (`"a,b"`, escaped quotes).

Trade-like data:

```csv
symbol,timestamp,price,quantity
AAPL,1600000000000000000,185.12,100
```

Non-trade data:

```csv
user_id,country,age,score,is_paid
1,US,34,91.2,true
```

Current strategies:

- `linear_scan`
- `hash_index` for equality filters
- `composite_hash` for multi-equality AND queries
- `sorted_index` for numeric range filters (binary search bounds)
- `columnar_scan` for column-major layout comparison
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

Weighted workload file:

```powershell
.\build\Debug\queryforge.exe run --input examples\trades.csv --workload examples\workloads\trades.yaml --strategy all
```

Compare two JSON benchmark runs:

```powershell
.\build\Debug\queryforge.exe compare baseline.json candidate.json
```

## Benchmark Methodology

- Use `--warmup` to exclude cold-start samples from timed runs.
- Use `--runs` to control sample size; larger values reduce variance.
- Latency numbers use `steady_clock` and are best compared on the same machine and build type.
- CI runners are noisy; use local Release builds for meaningful timing comparisons.
- Match counts and strategy applicability are stable regression signals in CI.

## Prerequisites

- CMake 3.20 or newer
- A C++20 compiler (MSVC 2019+, GCC 10+, or Clang 12+)

On Windows with the Visual Studio generator, the executable is typically at `build\Debug\queryforge.exe` or `build\Release\queryforge.exe`. With Ninja or other single-config generators, it is at `build\queryforge.exe`.

## Tests

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

## Releases

Tagged releases (`v*`) publish Windows, Linux, and macOS binaries through GitHub Releases. See [CHANGELOG.md](CHANGELOG.md) for version history.

Package templates:

- [packaging/homebrew/queryforge.rb](packaging/homebrew/queryforge.rb)
- [packaging/winget/queryforge.yaml](packaging/winget/queryforge.yaml)
- [Dockerfile](Dockerfile)

## Roadmap

Completed in v1.0:

- CSV input with schema inference and schema files
- Query filters on arbitrary columns
- Strategy comparison with recommendations
- JSON output, inspect/compare commands, and CI

Future work:

- SIMD and parallel scan strategies
- Streaming load for files larger than RAM
- Published Homebrew/Winget packages tied to your GitHub org URL

## Project Layout

```text
QueryForge/
├── CMakeLists.txt
├── include/queryforge/   # public headers
├── src/                  # implementations
├── tests/                # unit tests (Catch2)
├── benchmarks/fixtures/  # regression datasets
├── examples/             # sample datasets and workloads
└── .github/              # CI and issue templates
```
