# QueryForge

[![CI](https://github.com/jaigudla/QueryForge/actions/workflows/ci.yml/badge.svg)](https://github.com/jaigudla/QueryForge/actions/workflows/ci.yml)

QueryForge is a C++20 command-line tool for benchmarking in-memory query strategies. Load CSV or synthetic data, run filters against multiple index layouts, and get latency distributions, memory estimates, and a strategy recommendation.

**Stack:** C++20 · CMake · CLI11 · Catch2 · GitHub Actions

## What it does

1. Load a typed in-memory table from CSV (inferred or explicit schema) or generate synthetic data.
2. Run one or more `--where` filters, or a weighted `--workload` file.
3. Benchmark applicable strategies and compare build cost, query latency (p50/p95/p99), and memory.
4. Print a recommendation based on expected total cost for repeated queries.

## Install

**Prerequisites:** CMake 3.20+, C++20 compiler (MSVC 2019+, GCC 10+, or Clang 12+)

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix install   # optional
```

On Windows with Visual Studio, the binary is usually `build\Debug\queryforge.exe` or `build\Release\queryforge.exe`. With Ninja or Make, it is `build/queryforge`.

## Commands

| Command | Purpose |
|---------|---------|
| `queryforge run` | Benchmark query strategies |
| `queryforge inspect <file>` | Preview schema and sample rows |
| `queryforge compare <baseline> <candidate>` | Diff two JSON benchmark outputs |
| `queryforge --version` | Print version |

Run `queryforge <command> --help` for all flags.

## Quick examples

Benchmark trades CSV with all strategies:

```bash
queryforge run --input examples/trades.csv --where symbol=AAPL --strategy all --runs 30
```

Inspect inferred schema:

```bash
queryforge inspect examples/users.csv
```

Arbitrary schema with explicit types:

```bash
queryforge run --input examples/users.csv --where country=US --where age>=30 --strategy all
```

Synthetic data (trade-shaped by default, or pass `--schema`):

```bash
queryforge run --rows 1000000 --where symbol=AAPL --strategy all --warmup 5 --runs 30 --seed 42
queryforge run --rows 100000 --schema user_id:int64,country:string,score:double --where country=US --strategy all
```

JSON output for scripting:

```bash
queryforge run --input examples/trades.csv --where symbol=AAPL --strategy all --output json
```

Weighted multi-query workload:

```bash
queryforge run --input examples/trades.csv --workload examples/workloads/trades.yaml --strategy all
```

Example text output:

```text
Result:
Strategy      build     p50       p95       p99       avg       stddev    memory      matches
-----------------------------------------------------------------------------------------------
linear_scan   0.0 ms    23.4 ms   26.8 ms   30.1 ms   24.0 ms   1.2 ms    0 B         1,955
hash_index    7.2 ms    0.1 ms    0.1 ms    0.2 ms    0.1 ms    0.0 ms    8.1 MB      1,955

Recommendation:
  Use hash_index for this workload. Estimated total cost for 1 repeated queries is 7.3 ms.
```

## Data input

CSV files must have a header row. QueryForge infers column types by default, or you can pass:

- `--schema name:type,...` — e.g. `user_id:int64,country:string`
- `--schema-file path` — JSON or line-based schema (see `examples/schemas/users.json`)
- `--delimiter` — single-character delimiter (default `,`)
- `--no-infer-schema` — require an explicit schema

Supported column types: `string`, `int64`, `double`, `bool`. Quoted fields and escaped quotes are supported.

## Query filters

Use `--where` with `=`, `<`, `<=`, `>`, `>=`:

```text
symbol=AAPL
country=US
age>=30
price<250
frame_time_ms>20
is_paid=true
```

`--symbol` is a deprecated shortcut for `--where symbol=<value>` on trade-shaped data.

## Strategies

| Strategy | Best for |
|----------|----------|
| `linear_scan` | Baseline full-table scan |
| `hash_index` | Single equality filter |
| `composite_hash` | Multiple equality filters (AND) |
| `sorted_index` | Numeric range filters (binary search on sorted column) |
| `columnar_scan` | Column-major layout comparison |
| `all` | Run every strategy that applies to the query |

Pass `--repeat-count N` to model repeated queries in the recommendation. Use `--memory-profile` to report peak RSS.

## Benchmark notes

- `--warmup` excludes cold-start iterations; `--runs` sets the sample count.
- Timings use `steady_clock`. Compare results on the same machine and build type.
- CI runners are noisy — use local Release builds for meaningful latency numbers.
- Match counts and strategy applicability are validated in tests; absolute timings are not gated in CI.

## Tests

```bash
ctest --test-dir build -C Debug --output-on-failure
```

Single-config generators (Ninja, Make) can omit `-C Debug`.

## Releases

Tag pushes (`v*`) build Windows, Linux, and macOS binaries via GitHub Actions. See [CHANGELOG.md](CHANGELOG.md).

Optional distribution templates: [Dockerfile](Dockerfile), [Homebrew formula](packaging/homebrew/queryforge.rb), [Winget manifest](packaging/winget/queryforge.yaml).

## Project layout

```text
QueryForge/
├── include/queryforge/   # Public headers
├── src/                  # Library and CLI implementation
├── tests/                # Catch2 unit and regression tests
├── benchmarks/fixtures/  # Stable CSV fixtures for CI
├── examples/             # Sample CSVs, schemas, and workloads
└── .github/              # CI, release, and issue templates
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Bug reports and workload ideas welcome via GitHub issues.
