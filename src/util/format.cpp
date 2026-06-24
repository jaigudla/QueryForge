#include "queryforge/util/format.hpp"

#include <cmath>
#include <format>
#include <iomanip>
#include <iostream>
#include <string>

namespace queryforge {

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

std::string format_bytes(std::size_t bytes) {
    constexpr double kib = 1024.0;
    constexpr double mib = kib * 1024.0;
    constexpr double gib = mib * 1024.0;

    if (bytes >= static_cast<std::size_t>(gib)) {
        return std::format("{:.1f} GB", static_cast<double>(bytes) / gib);
    }
    if (bytes >= static_cast<std::size_t>(mib)) {
        return std::format("{:.1f} MB", static_cast<double>(bytes) / mib);
    }
    if (bytes >= static_cast<std::size_t>(kib)) {
        return std::format("{:.1f} KB", static_cast<double>(bytes) / kib);
    }
    return std::format("{} B", bytes);
}

void print_benchmark_table(const std::string& strategy,
                           const BenchmarkStats& stats,
                           std::size_t matches) {
    constexpr int strategy_width = 14;
    constexpr int metric_width = 10;

    std::cout << std::left << std::setw(strategy_width) << "Strategy"
              << std::setw(metric_width) << "p50"
              << std::setw(metric_width) << "p95"
              << std::setw(metric_width) << "p99"
              << std::setw(metric_width) << "avg"
              << "matches" << '\n';
    std::cout << "---------------------------------------------------------------\n";
    std::cout << std::left << std::setw(strategy_width) << strategy
              << std::setw(metric_width) << format_ms(stats.p50_ms)
              << std::setw(metric_width) << format_ms(stats.p95_ms)
              << std::setw(metric_width) << format_ms(stats.p99_ms)
              << std::setw(metric_width) << format_ms(stats.avg_ms)
              << format_with_commas(static_cast<std::uint64_t>(matches)) << '\n';
}

void print_strategy_table(const std::vector<StrategyBenchmarkResult>& results) {
    constexpr int strategy_width = 14;
    constexpr int metric_width = 10;
    constexpr int memory_width = 12;

    std::cout << std::left << std::setw(strategy_width) << "Strategy"
              << std::setw(metric_width) << "build"
              << std::setw(metric_width) << "p50"
              << std::setw(metric_width) << "p95"
              << std::setw(metric_width) << "p99"
              << std::setw(metric_width) << "avg"
              << std::setw(metric_width) << "stddev"
              << std::setw(memory_width) << "memory"
              << "matches" << '\n';
    std::cout << "-----------------------------------------------------------------------------------------------\n";

    for (const StrategyBenchmarkResult& result : results) {
        if (!result.applicable) {
            std::cout << std::left << std::setw(strategy_width) << result.strategy
                      << result.note << '\n';
            continue;
        }

        std::cout << std::left << std::setw(strategy_width) << result.strategy
                  << std::setw(metric_width) << format_ms(result.build_time_ms)
                  << std::setw(metric_width) << format_ms(result.stats.p50_ms)
                  << std::setw(metric_width) << format_ms(result.stats.p95_ms)
                  << std::setw(metric_width) << format_ms(result.stats.p99_ms)
                  << std::setw(metric_width) << format_ms(result.stats.avg_ms)
                  << std::setw(metric_width) << format_ms(result.stats.stddev_ms)
                  << std::setw(memory_width) << format_bytes(result.memory_bytes)
                  << format_with_commas(static_cast<std::uint64_t>(result.matches)) << '\n';
    }
}

}  // namespace queryforge
