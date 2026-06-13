#include "queryforge/query/linear_scan.hpp"

#include "queryforge/benchmark/benchmark_stats.hpp"
#include "queryforge/query/query_filter.hpp"

#include <chrono>
#include <vector>

namespace {

std::size_t count_matches(const std::vector<TradeEvent>& events, const QuerySpec& query) {
    std::size_t matches = 0;
    for (const TradeEvent& event : events) {
        if (matches_query(event, query)) {
            ++matches;
        }
    }
    return matches;
}

}  // namespace

ScanResult linear_scan_benchmark(const std::vector<TradeEvent>& events,
                                 const QuerySpec& query,
                                 int runs,
                                 int warmup) {
    const std::size_t matches = count_matches(events, query);

    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(runs));

    volatile std::size_t observed = 0;
    for (int i = 0; i < warmup + runs; ++i) {
        const auto start = std::chrono::steady_clock::now();
        observed = count_matches(events, query);
        const auto end = std::chrono::steady_clock::now();

        if (i >= warmup) {
            const double elapsed_ms =
                std::chrono::duration<double, std::milli>(end - start).count();
            samples.push_back(elapsed_ms);
        }
    }

    return ScanResult{matches, compute_benchmark_stats(samples)};
}

ScanResult linear_scan_benchmark(const std::vector<TradeEvent>& events,
                                 const std::string& symbol,
                                 int runs,
                                 int warmup) {
    return linear_scan_benchmark(events, parse_query_filters({"symbol=" + symbol}), runs, warmup);
}
