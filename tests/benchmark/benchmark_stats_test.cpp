#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "queryforge/benchmark/benchmark_stats.hpp"

#include <stdexcept>
#include <vector>

TEST_CASE("compute_benchmark_stats calculates basic distribution values", "[benchmark_stats]") {
    const BenchmarkStats stats = compute_benchmark_stats({5.0, 1.0, 4.0, 2.0, 3.0});

    REQUIRE(stats.latencies_ms == std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
    REQUIRE(stats.p50_ms == 3.0);
    REQUIRE(stats.avg_ms == 3.0);
    REQUIRE(stats.min_ms == 1.0);
    REQUIRE(stats.max_ms == 5.0);
}

TEST_CASE("compute_benchmark_stats interpolates p95 and p99", "[benchmark_stats]") {
    const BenchmarkStats stats = compute_benchmark_stats({1.0, 2.0, 3.0, 4.0, 5.0});

    REQUIRE(stats.p95_ms == Catch::Approx(4.8));
    REQUIRE(stats.p99_ms == Catch::Approx(4.96));
}

TEST_CASE("compute_benchmark_stats rejects empty samples", "[benchmark_stats]") {
    REQUIRE_THROWS_AS(compute_benchmark_stats({}), std::invalid_argument);
}
