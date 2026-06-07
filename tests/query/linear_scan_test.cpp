#include <catch2/catch_test_macros.hpp>

#include "queryforge/query/linear_scan.hpp"

TEST_CASE("linear_scan counts matching symbols", "[linear_scan]") {
    const std::vector<TradeEvent> events = {
        {"AAPL", 1, 100.0, 10},
        {"MSFT", 2, 200.0, 20},
        {"AAPL", 3, 150.0, 30},
        {"NVDA", 4, 300.0, 40},
        {"AAPL", 5, 125.0, 50},
    };

    const ScanResult result = linear_scan_benchmark(events, "AAPL", 1, 0);
    REQUIRE(result.matches == 3);
}

TEST_CASE("linear_scan reports positive latency", "[linear_scan]") {
    const std::vector<TradeEvent> events = {
        {"AAPL", 1, 100.0, 10},
        {"MSFT", 2, 200.0, 20},
    };

    const ScanResult result = linear_scan_benchmark(events, "AAPL", 5, 1);
    REQUIRE(result.p50_ms > 0.0);
}
