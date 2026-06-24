#include "queryforge/benchmark/benchmark_stats.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace queryforge {

double percentile(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) {
        throw std::invalid_argument("percentile requires at least one sample");
    }

    const double clamped = std::clamp(p, 0.0, 100.0);
    const double rank = (clamped / 100.0) * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(rank));
    const auto upper = static_cast<std::size_t>(std::ceil(rank));

    if (lower == upper) {
        return sorted[lower];
    }

    const double weight = rank - static_cast<double>(lower);
    return sorted[lower] + (sorted[upper] - sorted[lower]) * weight;
}

BenchmarkStats compute_benchmark_stats(std::vector<double> latencies_ms) {
    if (latencies_ms.empty()) {
        throw std::invalid_argument("benchmark stats require at least one latency sample");
    }

    std::sort(latencies_ms.begin(), latencies_ms.end());

    const double total = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0);
    const double avg = total / static_cast<double>(latencies_ms.size());

    double variance = 0.0;
    for (const double sample : latencies_ms) {
        const double delta = sample - avg;
        variance += delta * delta;
    }
    const double stddev = std::sqrt(variance / static_cast<double>(latencies_ms.size()));

    return BenchmarkStats{
        latencies_ms,
        percentile(latencies_ms, 50.0),
        percentile(latencies_ms, 95.0),
        percentile(latencies_ms, 99.0),
        avg,
        stddev,
        latencies_ms.front(),
        latencies_ms.back(),
    };
}

}  // namespace queryforge
