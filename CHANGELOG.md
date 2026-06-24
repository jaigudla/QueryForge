# Changelog

All notable changes to QueryForge are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-06-23

### Added

- `queryforge --version` and installable CMake targets
- `queryforge inspect` for schema preview and sample rows
- `--schema-file` for external schema definitions
- Quoted CSV field parsing with escaped quotes
- Schema-driven synthetic data generation via `--rows` and `--schema`
- `composite_hash` and `columnar_scan` query strategies
- `--workload` for weighted multi-query benchmarks
- `queryforge compare` for diffing JSON benchmark runs
- `queryforge` namespace and `Result<T>` error boundary at the CLI
- AddressSanitizer/UBSan CI job on Linux
- Optional CSV parser fuzz target
- Benchmark stddev in stats output and JSON regression tests
- GitHub Release automation with cross-platform binaries
- `SECURITY.md` and `CODE_OF_CONDUCT.md`
- Docker image and package manifest templates (Homebrew, Winget)

### Fixed

- `sorted_index` now uses binary search to seek range bounds instead of scanning all rows

### Changed

- CLI help text describes generic table workloads instead of trade-only CSV
- README roadmap updated to reflect shipped features

## [0.1.0] - 2026-06-23

### Added

- Initial release: typed `Table` model, CSV loading with schema inference
- Query filters (`--where`) on arbitrary columns
- Strategies: `linear_scan`, `hash_index`, `sorted_index`
- Benchmark latency distributions (p50, p95, p99, avg, min, max)
- Strategy recommendations with build cost and memory estimates
- JSON and text output modes
- Cross-platform CI and example datasets
