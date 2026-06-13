#include "queryforge/query/strategy.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace {

using QueryFn = std::function<std::size_t()>;

double elapsed_ms(std::chrono::steady_clock::time_point start,
                  std::chrono::steady_clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

BenchmarkStats benchmark_query(int runs, int warmup, const QueryFn& query_fn) {
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(runs));

    volatile std::size_t observed = 0;
    for (int i = 0; i < warmup + runs; ++i) {
        const auto start = std::chrono::steady_clock::now();
        observed = query_fn();
        const auto end = std::chrono::steady_clock::now();
        if (i >= warmup) {
            samples.push_back(elapsed_ms(start, end));
        }
    }
    (void)observed;

    return compute_benchmark_stats(samples);
}

std::string key_for_cell(const CellValue& value) {
    return std::visit(
        [](const auto& current) -> std::string {
            using T = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return "s:" + current;
            } else if constexpr (std::is_same_v<T, bool>) {
                return current ? "b:true" : "b:false";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return "i:" + std::to_string(current);
            } else {
                return "d:" + std::to_string(current);
            }
        },
        value);
}

std::size_t count_linear(const Table& table, const QuerySpec& query) {
    std::size_t matches = 0;
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (matches_query(table, row, query)) {
            ++matches;
        }
    }
    return matches;
}

StrategyBenchmarkResult benchmark_linear_scan(const Table& table,
                                              const QuerySpec& query,
                                              int runs,
                                              int warmup) {
    const std::size_t matches = count_linear(table, query);
    return StrategyBenchmarkResult{
        "linear_scan",
        0.0,
        benchmark_query(runs, warmup, [&] { return count_linear(table, query); }),
        matches,
        0,
        true,
        {},
    };
}

StrategyBenchmarkResult benchmark_hash_index(const Table& table,
                                             const QuerySpec& query,
                                             int runs,
                                             int warmup) {
    QueryFilter equality_filter{};
    if (!find_equality_filter(query, equality_filter)) {
        return StrategyBenchmarkResult{
            "hash_index",
            0.0,
            compute_benchmark_stats({0.0}),
            0,
            0,
            false,
            "requires an equality filter",
        };
    }

    const auto build_start = std::chrono::steady_clock::now();
    std::unordered_map<std::string, std::vector<std::size_t>> index;
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
        index[key_for_cell(table.rows[i][equality_filter.column_index])].push_back(i);
    }
    const auto build_end = std::chrono::steady_clock::now();

    const std::string lookup_key = key_for_cell(equality_filter.value);
    const auto query_fn = [&] {
        std::size_t matches = 0;
        const auto found = index.find(lookup_key);
        if (found == index.end()) {
            return matches;
        }
        for (const std::size_t row : found->second) {
            if (matches_query(table, row, query)) {
                ++matches;
            }
        }
        return matches;
    };

    std::size_t indexed_rows = 0;
    for (const auto& [_, rows] : index) {
        indexed_rows += rows.capacity();
    }

    return StrategyBenchmarkResult{
        "hash_index",
        elapsed_ms(build_start, build_end),
        benchmark_query(runs, warmup, query_fn),
        query_fn(),
        index.bucket_count() * sizeof(void*) + indexed_rows * sizeof(std::size_t),
        true,
        {},
    };
}

StrategyBenchmarkResult benchmark_sorted_index(const Table& table,
                                               const QuerySpec& query,
                                               int runs,
                                               int warmup) {
    QueryFilter range_filter{};
    if (!find_range_filter(query, range_filter)) {
        return StrategyBenchmarkResult{
            "sorted_index",
            0.0,
            compute_benchmark_stats({0.0}),
            0,
            0,
            false,
            "requires a numeric range filter",
        };
    }

    const auto build_start = std::chrono::steady_clock::now();
    std::vector<std::size_t> sorted_rows(table.rows.size());
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
        sorted_rows[i] = i;
    }
    std::sort(sorted_rows.begin(), sorted_rows.end(), [&](std::size_t lhs, std::size_t rhs) {
        return cell_to_number(table.rows[lhs][range_filter.column_index]) <
               cell_to_number(table.rows[rhs][range_filter.column_index]);
    });
    const auto build_end = std::chrono::steady_clock::now();

    const auto query_fn = [&] {
        std::size_t matches = 0;
        for (const std::size_t row : sorted_rows) {
            if (matches_query(table, row, query)) {
                ++matches;
            }
        }
        return matches;
    };

    return StrategyBenchmarkResult{
        "sorted_index",
        elapsed_ms(build_start, build_end),
        benchmark_query(runs, warmup, query_fn),
        query_fn(),
        sorted_rows.capacity() * sizeof(std::size_t),
        true,
        "sorted by " + range_filter.column,
    };
}

}  // namespace

std::vector<std::string> normalize_strategy_names(const std::vector<std::string>& requested) {
    if (requested.empty() ||
        std::find(requested.begin(), requested.end(), "all") != requested.end()) {
        return {"linear_scan", "hash_index", "sorted_index"};
    }

    for (const std::string& strategy : requested) {
        if (strategy != "linear_scan" && strategy != "hash_index" && strategy != "sorted_index") {
            throw std::runtime_error("Unknown strategy: " + strategy);
        }
    }
    return requested;
}

std::vector<StrategyBenchmarkResult> benchmark_strategies(const Table& table,
                                                          const QuerySpec& query,
                                                          const std::vector<std::string>& strategies,
                                                          int runs,
                                                          int warmup) {
    std::vector<StrategyBenchmarkResult> results;
    for (const std::string& strategy : normalize_strategy_names(strategies)) {
        if (strategy == "linear_scan") {
            results.push_back(benchmark_linear_scan(table, query, runs, warmup));
        } else if (strategy == "hash_index") {
            results.push_back(benchmark_hash_index(table, query, runs, warmup));
        } else if (strategy == "sorted_index") {
            results.push_back(benchmark_sorted_index(table, query, runs, warmup));
        }
    }
    return results;
}
