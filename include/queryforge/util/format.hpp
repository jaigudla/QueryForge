#pragma once

#include "queryforge/benchmark/benchmark_stats.hpp"
#include "queryforge/query/strategy.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace queryforge {

std::string format_with_commas(std::uint64_t value);
std::string format_ms(double ms);
std::string format_bytes(std::size_t bytes);
void print_benchmark_table(const std::string& strategy,
                           const BenchmarkStats& stats,
                           std::size_t matches);
void print_strategy_table(const std::vector<StrategyBenchmarkResult>& results);

}  // namespace queryforge
