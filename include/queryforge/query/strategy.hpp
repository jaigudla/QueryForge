#pragma once

#include "queryforge/benchmark/benchmark_stats.hpp"
#include "queryforge/core/trade_event.hpp"
#include "queryforge/query/query_filter.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct StrategyBenchmarkResult {
    std::string strategy;
    double build_time_ms = 0.0;
    BenchmarkStats stats;
    std::size_t matches = 0;
    std::size_t memory_bytes = 0;
    bool applicable = true;
    std::string note;
};

std::vector<std::string> normalize_strategy_names(const std::vector<std::string>& requested);
std::vector<StrategyBenchmarkResult> benchmark_strategies(const std::vector<TradeEvent>& events,
                                                          const QuerySpec& query,
                                                          const std::vector<std::string>& strategies,
                                                          int runs,
                                                          int warmup);
