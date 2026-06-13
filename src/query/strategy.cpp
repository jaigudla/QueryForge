#include "queryforge/query/strategy.hpp"

#include "queryforge/query/linear_scan.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <stdexcept>
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

std::size_t count_linear(const std::vector<TradeEvent>& events, const QuerySpec& query) {
    std::size_t matches = 0;
    for (const TradeEvent& event : events) {
        if (matches_query(event, query)) {
            ++matches;
        }
    }
    return matches;
}

StrategyBenchmarkResult benchmark_linear_scan(const std::vector<TradeEvent>& events,
                                              const QuerySpec& query,
                                              int runs,
                                              int warmup) {
    const std::size_t matches = count_linear(events, query);
    return StrategyBenchmarkResult{
        "linear_scan",
        0.0,
        benchmark_query(runs, warmup, [&] { return count_linear(events, query); }),
        matches,
        0,
        true,
        {},
    };
}

StrategyBenchmarkResult benchmark_hash_index(const std::vector<TradeEvent>& events,
                                             const QuerySpec& query,
                                             int runs,
                                             int warmup) {
    std::string symbol;
    if (!find_symbol_equality(query, symbol)) {
        return StrategyBenchmarkResult{
            "hash_index",
            0.0,
            compute_benchmark_stats({0.0}),
            0,
            0,
            false,
            "requires symbol equality filter",
        };
    }

    const auto build_start = std::chrono::steady_clock::now();
    std::unordered_map<std::string, std::vector<std::size_t>> index;
    for (std::size_t i = 0; i < events.size(); ++i) {
        index[events[i].symbol].push_back(i);
    }
    const auto build_end = std::chrono::steady_clock::now();

    const auto query_fn = [&] {
        std::size_t matches = 0;
        const auto found = index.find(symbol);
        if (found == index.end()) {
            return matches;
        }
        for (const std::size_t row : found->second) {
            if (matches_query(events[row], query)) {
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

double numeric_value(const TradeEvent& event, QueryField field) {
    switch (field) {
        case QueryField::Timestamp:
            return static_cast<double>(event.timestamp);
        case QueryField::Price:
            return event.price;
        case QueryField::Quantity:
            return static_cast<double>(event.quantity);
        case QueryField::Symbol:
            return 0.0;
    }
    return 0.0;
}

bool find_sorted_field(const QuerySpec& query, QueryField& field) {
    for (const QueryField candidate : {QueryField::Timestamp, QueryField::Price, QueryField::Quantity}) {
        if (has_range_filter(query, candidate)) {
            field = candidate;
            return true;
        }
    }
    return false;
}

StrategyBenchmarkResult benchmark_sorted_index(const std::vector<TradeEvent>& events,
                                               const QuerySpec& query,
                                               int runs,
                                               int warmup) {
    QueryField field = QueryField::Timestamp;
    if (!find_sorted_field(query, field)) {
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
    std::vector<std::size_t> sorted_rows(events.size());
    for (std::size_t i = 0; i < events.size(); ++i) {
        sorted_rows[i] = i;
    }
    std::sort(sorted_rows.begin(), sorted_rows.end(), [&](std::size_t lhs, std::size_t rhs) {
        return numeric_value(events[lhs], field) < numeric_value(events[rhs], field);
    });
    const auto build_end = std::chrono::steady_clock::now();

    const auto query_fn = [&] {
        std::size_t matches = 0;
        for (const std::size_t row : sorted_rows) {
            if (matches_query(events[row], query)) {
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
        "sorted by " + field_name(field),
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

std::vector<StrategyBenchmarkResult> benchmark_strategies(const std::vector<TradeEvent>& events,
                                                          const QuerySpec& query,
                                                          const std::vector<std::string>& strategies,
                                                          int runs,
                                                          int warmup) {
    std::vector<StrategyBenchmarkResult> results;
    for (const std::string& strategy : normalize_strategy_names(strategies)) {
        if (strategy == "linear_scan") {
            results.push_back(benchmark_linear_scan(events, query, runs, warmup));
        } else if (strategy == "hash_index") {
            results.push_back(benchmark_hash_index(events, query, runs, warmup));
        } else if (strategy == "sorted_index") {
            results.push_back(benchmark_sorted_index(events, query, runs, warmup));
        }
    }
    return results;
}
