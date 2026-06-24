#include "queryforge/recommend/recommendation.hpp"

#include "queryforge/util/format.hpp"

#include <limits>

namespace queryforge {

Recommendation make_recommendation(const std::vector<StrategyBenchmarkResult>& results,
                                   std::size_t repeat_count) {
    const StrategyBenchmarkResult* best = nullptr;
    double best_total = std::numeric_limits<double>::max();

    for (const StrategyBenchmarkResult& result : results) {
        if (!result.applicable) {
            continue;
        }
        const double total = result.build_time_ms +
                             (static_cast<double>(repeat_count) * result.stats.avg_ms);
        if (total < best_total ||
            (total == best_total && best != nullptr && result.memory_bytes < best->memory_bytes)) {
            best = &result;
            best_total = total;
        }
    }

    if (best == nullptr) {
        return Recommendation{
            "none",
            "No requested strategy can satisfy this query shape.",
            0.0,
        };
    }

    std::string message = "Use " + best->strategy + " for this workload. Estimated total cost for " +
                          format_with_commas(static_cast<std::uint64_t>(repeat_count)) +
                          " repeated queries is " + format_ms(best_total) + ".";
    if (best->build_time_ms > 0.0) {
        message += " This includes " + format_ms(best->build_time_ms) + " of index build time.";
    }
    if (best->memory_bytes > 0) {
        message += " Extra memory is about " + format_bytes(best->memory_bytes) + ".";
    }

    return Recommendation{best->strategy, message, best_total};
}

Recommendation make_weighted_recommendation(
    const std::vector<WeightedStrategyResult>& results,
    std::size_t repeat_count) {
    const WeightedStrategyResult* best = nullptr;
    double best_total = std::numeric_limits<double>::max();

    for (const WeightedStrategyResult& result : results) {
        if (!result.applicable) {
            continue;
        }
        const double total = result.build_time_ms +
                             (static_cast<double>(repeat_count) * result.weighted_avg_ms);
        if (total < best_total ||
            (total == best_total && best != nullptr && result.memory_bytes < best->memory_bytes)) {
            best = &result;
            best_total = total;
        }
    }

    if (best == nullptr) {
        return Recommendation{
            "none",
            "No requested strategy can satisfy this workload.",
            0.0,
        };
    }

    std::string message = "Use " + best->strategy + " for this workload. Estimated total cost for " +
                          format_with_commas(static_cast<std::uint64_t>(repeat_count)) +
                          " weighted queries is " + format_ms(best_total) + ".";
    if (best->build_time_ms > 0.0) {
        message += " This includes " + format_ms(best->build_time_ms) + " of index build time.";
    }
    if (best->memory_bytes > 0) {
        message += " Extra memory is about " + format_bytes(best->memory_bytes) + ".";
    }

    return Recommendation{best->strategy, message, best_total};
}

}  // namespace queryforge
