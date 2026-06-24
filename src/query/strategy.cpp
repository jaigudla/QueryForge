#include "queryforge/query/strategy.hpp"

#include <algorithm>
#include <algorithm>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace queryforge {
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

std::string composite_key_for_row(const Table& table,
                                  std::size_t row_index,
                                  const std::vector<QueryFilter>& equality_filters) {
    std::string key;
    for (const QueryFilter& filter : equality_filters) {
        if (!key.empty()) {
            key.push_back('|');
        }
        key += key_for_cell(table.rows[row_index][filter.column_index]);
    }
    return key;
}

double sort_value(const Table& table, std::size_t row_index, std::size_t column_index) {
    return cell_to_number(table.rows[row_index][column_index]);
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

std::pair<std::size_t, std::size_t> compute_scan_range(const Table& table,
                                                       const std::vector<std::size_t>& sorted_rows,
                                                       const QuerySpec& query,
                                                       const QueryFilter& primary_range) {
    std::size_t start = 0;
    std::size_t end = sorted_rows.size();
    const std::size_t column = primary_range.column_index;

    for (const QueryFilter& filter : query.filters) {
        if (filter.column_index != column ||
            (filter.type != ColumnType::Int64 && filter.type != ColumnType::Double) ||
            filter.op == QueryOperator::Equal) {
            continue;
        }

        const double bound = cell_to_number(filter.value);
        const auto value_at = [&](std::size_t row) {
            return sort_value(table, row, column);
        };
        if (filter.op == QueryOperator::GreaterEqual) {
            auto it = std::lower_bound(
                sorted_rows.begin() + start, sorted_rows.begin() + end, bound,
                [&](std::size_t row, double rhs) { return value_at(row) < rhs; });
            start = static_cast<std::size_t>(it - sorted_rows.begin());
        } else if (filter.op == QueryOperator::Greater) {
            auto it = std::upper_bound(
                sorted_rows.begin() + start, sorted_rows.begin() + end, bound,
                [&](double lhs, std::size_t row) { return lhs < value_at(row); });
            start = static_cast<std::size_t>(it - sorted_rows.begin());
        } else if (filter.op == QueryOperator::LessEqual) {
            auto it = std::upper_bound(
                sorted_rows.begin() + start, sorted_rows.begin() + end, bound,
                [&](double lhs, std::size_t row) { return lhs < value_at(row); });
            end = static_cast<std::size_t>(it - sorted_rows.begin());
        } else if (filter.op == QueryOperator::Less) {
            auto it = std::lower_bound(
                sorted_rows.begin() + start, sorted_rows.begin() + end, bound,
                [&](std::size_t row, double rhs) { return value_at(row) < rhs; });
            end = static_cast<std::size_t>(it - sorted_rows.begin());
        }
    }

    return {start, end};
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
        indexed_rows += rows.size();
    }

    return StrategyBenchmarkResult{
        "hash_index",
        elapsed_ms(build_start, build_end),
        benchmark_query(runs, warmup, query_fn),
        query_fn(),
        index.bucket_count() * sizeof(void*) + indexed_rows * sizeof(std::size_t) +
            index.size() * sizeof(std::string),
        true,
        "indexed on " + equality_filter.column,
    };
}

StrategyBenchmarkResult benchmark_composite_hash(const Table& table,
                                                 const QuerySpec& query,
                                                 int runs,
                                                 int warmup) {
    std::vector<QueryFilter> equality_filters;
    if (!find_all_equality_filters(query, equality_filters) || equality_filters.size() < 2) {
        return StrategyBenchmarkResult{
            "composite_hash",
            0.0,
            compute_benchmark_stats({0.0}),
            0,
            0,
            false,
            "requires two or more equality filters",
        };
    }

    const auto build_start = std::chrono::steady_clock::now();
    std::unordered_map<std::string, std::vector<std::size_t>> index;
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
        index[composite_key_for_row(table, i, equality_filters)].push_back(i);
    }
    const auto build_end = std::chrono::steady_clock::now();

    const std::string lookup_key = [&] {
        std::string key;
        for (const QueryFilter& filter : equality_filters) {
            if (!key.empty()) {
                key.push_back('|');
            }
            key += key_for_cell(filter.value);
        }
        return key;
    }();

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
        indexed_rows += rows.size();
    }

    return StrategyBenchmarkResult{
        "composite_hash",
        elapsed_ms(build_start, build_end),
        benchmark_query(runs, warmup, query_fn),
        query_fn(),
        index.bucket_count() * sizeof(void*) + indexed_rows * sizeof(std::size_t) +
            index.size() * sizeof(std::string),
        true,
        "composite key over equality filters",
    };
}

StrategyBenchmarkResult benchmark_sorted_index(const Table& table,
                                               const QuerySpec& query,
                                               int runs,
                                               int warmup) {
    QueryFilter range_filter{};
    if (!find_best_range_filter(query, range_filter)) {
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
        return sort_value(table, lhs, range_filter.column_index) <
               sort_value(table, rhs, range_filter.column_index);
    });
    const auto build_end = std::chrono::steady_clock::now();

    const auto query_fn = [&] {
        const auto [start, end] = compute_scan_range(table, sorted_rows, query, range_filter);
        std::size_t matches = 0;
        for (std::size_t position = start; position < end; ++position) {
            const std::size_t row = sorted_rows[position];
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
        "sorted by " + range_filter.column + " with binary search bounds",
    };
}

StrategyBenchmarkResult benchmark_columnar_scan(const Table& table,
                                                const QuerySpec& query,
                                                int runs,
                                                int warmup) {
    const auto build_start = std::chrono::steady_clock::now();
    std::vector<std::vector<CellValue>> columns(table.schema.columns.size());
    for (std::size_t column = 0; column < columns.size(); ++column) {
        columns[column].reserve(table.rows.size());
        for (const std::vector<CellValue>& row : table.rows) {
            columns[column].push_back(row[column]);
        }
    }
    const auto build_end = std::chrono::steady_clock::now();

    const auto query_fn = [&] {
        std::size_t matches = 0;
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            bool ok = true;
            for (const QueryFilter& filter : query.filters) {
                (void)columns[filter.column_index][row];
                if (!matches_query(table, row, QuerySpec{{filter}})) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                ++matches;
            }
        }
        return matches;
    };

    std::size_t memory_bytes = 0;
    for (const std::vector<CellValue>& column : columns) {
        memory_bytes += column.capacity() * sizeof(CellValue);
    }

    return StrategyBenchmarkResult{
        "columnar_scan",
        elapsed_ms(build_start, build_end),
        benchmark_query(runs, warmup, query_fn),
        query_fn(),
        memory_bytes,
        true,
        "column-major layout",
    };
}

}  // namespace

std::vector<std::string> normalize_strategy_names(const std::vector<std::string>& requested) {
    if (requested.empty() ||
        std::find(requested.begin(), requested.end(), "all") != requested.end()) {
        return {"linear_scan", "hash_index", "composite_hash", "sorted_index", "columnar_scan"};
    }

    for (const std::string& strategy : requested) {
        if (strategy != "linear_scan" && strategy != "hash_index" && strategy != "composite_hash" &&
            strategy != "sorted_index" && strategy != "columnar_scan") {
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
        } else if (strategy == "composite_hash") {
            results.push_back(benchmark_composite_hash(table, query, runs, warmup));
        } else if (strategy == "sorted_index") {
            results.push_back(benchmark_sorted_index(table, query, runs, warmup));
        } else if (strategy == "columnar_scan") {
            results.push_back(benchmark_columnar_scan(table, query, runs, warmup));
        }
    }
    return results;
}

}  // namespace queryforge
