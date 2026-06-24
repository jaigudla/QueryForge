#pragma once

#include "queryforge/query/strategy.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace queryforge {

struct Recommendation {
    std::string strategy;
    std::string message;
    double estimated_total_ms = 0.0;
};

struct WeightedStrategyResult {
    std::string strategy;
    double weighted_avg_ms = 0.0;
    double build_time_ms = 0.0;
    std::size_t memory_bytes = 0;
    bool applicable = true;
};

Recommendation make_recommendation(const std::vector<StrategyBenchmarkResult>& results,
                                   std::size_t repeat_count);
Recommendation make_weighted_recommendation(
    const std::vector<WeightedStrategyResult>& results,
    std::size_t repeat_count);

}  // namespace queryforge
