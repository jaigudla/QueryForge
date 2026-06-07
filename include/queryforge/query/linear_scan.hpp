#pragma once

#include "queryforge/core/trade_event.hpp"

#include <string>
#include <vector>

struct ScanResult {
    std::size_t matches;
    double p50_ms;
};

ScanResult linear_scan_benchmark(const std::vector<TradeEvent>& events,
                                 const std::string& symbol,
                                 int iterations = 21,
                                 int warmup = 3);
