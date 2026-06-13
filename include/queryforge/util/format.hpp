#pragma once

#include "queryforge/benchmark/benchmark_stats.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

std::string format_with_commas(std::uint64_t value);
std::string format_ms(double ms);
void print_benchmark_table(const std::string& strategy,
                           const BenchmarkStats& stats,
                           std::size_t matches);
