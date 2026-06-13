#include "queryforge/util/format.hpp"

#include <cmath>
#include <format>
#include <iomanip>
#include <iostream>
#include <string>

std::string format_with_commas(std::uint64_t value) {
    const std::string digits = std::to_string(value);
    const int first_group = static_cast<int>(digits.size() % 3);
    std::string result;

    if (first_group > 0) {
        result.append(digits, 0, static_cast<std::size_t>(first_group));
    }

    for (int i = first_group; i < static_cast<int>(digits.size()); i += 3) {
        if (!result.empty()) {
            result.push_back(',');
        }
        result.append(digits, static_cast<std::size_t>(i), 3);
    }

    return result;
}

std::string format_ms(double ms) {
    const double rounded = std::round(ms * 10.0) / 10.0;
    return std::format("{:.1f} ms", rounded);
}

void print_benchmark_table(const std::string& strategy,
                           const BenchmarkStats& stats,
                           std::size_t matches) {
    constexpr int strategy_width = 14;
    constexpr int metric_width = 10;
    constexpr int matches_width = 0;

    std::cout << std::left << std::setw(strategy_width) << "Strategy"
              << std::setw(metric_width) << "p50"
              << std::setw(metric_width) << "p95"
              << std::setw(metric_width) << "p99"
              << std::setw(metric_width) << "avg"
              << std::setw(matches_width) << "matches" << '\n';
    std::cout << "---------------------------------------------------------------\n";
    std::cout << std::left << std::setw(strategy_width) << strategy
              << std::setw(metric_width) << format_ms(stats.p50_ms)
              << std::setw(metric_width) << format_ms(stats.p95_ms)
              << std::setw(metric_width) << format_ms(stats.p99_ms)
              << std::setw(metric_width) << format_ms(stats.avg_ms)
              << format_with_commas(static_cast<std::uint64_t>(matches)) << '\n';
}
