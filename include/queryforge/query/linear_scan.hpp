#pragma once

#include "queryforge/benchmark/benchmark_stats.hpp"
#include "queryforge/core/trade_event.hpp"

#include <string>
#include <vector>

struct ScanResult {
    std::size_t matches;
    BenchmarkStats stats;
};

ScanResult linear_scan_benchmark(const std::vector<TradeEvent>& events,
                                 const std::string& symbol,
                                 int runs = 30,
                                 int warmup = 5);
