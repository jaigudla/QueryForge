#pragma once

#include "queryforge/query/strategy.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct Recommendation {
    std::string strategy;
    std::string message;
    double estimated_total_ms = 0.0;
};

Recommendation make_recommendation(const std::vector<StrategyBenchmarkResult>& results,
                                   std::size_t repeat_count);
