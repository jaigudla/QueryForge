# Contributing To QueryForge

Thanks for helping improve QueryForge.

## Good First Contributions

- Share a real query pattern QueryForge should support.
- Add a small example dataset under `examples/`.
- Improve benchmark output clarity.
- Add tests for edge cases in CSV loading, query parsing, or strategy selection.

## Development Workflow

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build -C Debug --output-on-failure
```

With single-config generators, omit `-C Debug`.

## Code Style

- Keep headers under `include/queryforge/` and implementations under `src/`.
- Add focused Catch2 tests for new behavior.
- Prefer clear benchmark output over clever formatting.
- Keep CLI errors actionable for users trying the tool for the first time.

## Reporting Workloads

If QueryForge does not model your workload yet, open a workload request with:

- Dataset shape and approximate row count.
- Query pattern.
- Strategies you want compared.
- Whether queries are one-off or repeated many times.
