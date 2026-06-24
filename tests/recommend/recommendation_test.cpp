#include <catch2/catch_test_macros.hpp>

#include "queryforge/recommend/recommendation.hpp"

using namespace queryforge;

namespace {

BenchmarkStats stats(double avg_ms) {
    return BenchmarkStats{{avg_ms}, avg_ms, avg_ms, avg_ms, avg_ms, 0.0, avg_ms, avg_ms};
}

}  // namespace

TEST_CASE("make_recommendation chooses lowest total cost", "[recommendation]") {
    const std::vector<StrategyBenchmarkResult> results = {
        {"linear_scan", 0.0, stats(10.0), 10, 0, true, {}},
        {"hash_index", 20.0, stats(1.0), 10, 1024, true, {}},
    };

    const Recommendation once = make_recommendation(results, 1);
    const Recommendation repeated = make_recommendation(results, 100);

    REQUIRE(once.strategy == "linear_scan");
    REQUIRE(repeated.strategy == "hash_index");
}

TEST_CASE("make_recommendation ignores not applicable strategies", "[recommendation]") {
    const std::vector<StrategyBenchmarkResult> results = {
        {"hash_index", 0.0, stats(0.0), 0, 0, false, "not supported"},
    };

    const Recommendation recommendation = make_recommendation(results, 10);

    REQUIRE(recommendation.strategy == "none");
}
