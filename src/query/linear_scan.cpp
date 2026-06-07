#include "queryforge/query/linear_scan.hpp"

#include <algorithm>
#include <chrono>
#include <vector>

namespace {

std::size_t count_matches(const std::vector<TradeEvent>& events, const std::string& symbol) {
    std::size_t matches = 0;
    for (const TradeEvent& event : events) {
        if (event.symbol == symbol) {
            ++matches;
        }
    }
    return matches;
}

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const std::size_t mid = values.size() / 2;
    if (values.size() % 2 == 0) {
        return (values[mid - 1] + values[mid]) / 2.0;
    }
    return values[mid];
}

}  // namespace

ScanResult linear_scan_benchmark(const std::vector<TradeEvent>& events,
                                 const std::string& symbol,
                                 int iterations,
                                 int warmup) {
    const std::size_t matches = count_matches(events, symbol);

    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(iterations));

    for (int i = 0; i < warmup + iterations; ++i) {
        const auto start = std::chrono::steady_clock::now();
        count_matches(events, symbol);
        const auto end = std::chrono::steady_clock::now();

        if (i >= warmup) {
            const double elapsed_ms =
                std::chrono::duration<double, std::milli>(end - start).count();
            samples.push_back(elapsed_ms);
        }
    }

    return ScanResult{matches, median(samples)};
}
