#include <catch2/catch_test_macros.hpp>

#include "queryforge/query/strategy.hpp"

TEST_CASE("benchmark_strategies compares linear scan and hash index", "[strategy]") {
    const std::vector<TradeEvent> events = {
        {"AAPL", 1, 100.0, 10},
        {"MSFT", 2, 200.0, 20},
        {"AAPL", 3, 150.0, 30},
        {"NVDA", 4, 300.0, 40},
    };
    const QuerySpec query = parse_query_filters({"symbol=AAPL"});

    const auto results = benchmark_strategies(events, query, {"linear_scan", "hash_index"}, 1, 0);

    REQUIRE(results.size() == 2);
    REQUIRE(results[0].matches == 2);
    REQUIRE(results[1].matches == 2);
    REQUIRE(results[1].applicable);
}

TEST_CASE("benchmark_strategies marks unsupported sorted index as not applicable", "[strategy]") {
    const std::vector<TradeEvent> events = {
        {"AAPL", 1, 100.0, 10},
        {"MSFT", 2, 200.0, 20},
    };
    const QuerySpec query = parse_query_filters({"symbol=AAPL"});

    const auto results = benchmark_strategies(events, query, {"sorted_index"}, 1, 0);

    REQUIRE(results.size() == 1);
    REQUIRE_FALSE(results[0].applicable);
}

TEST_CASE("benchmark_strategies supports sorted index for range filters", "[strategy]") {
    const std::vector<TradeEvent> events = {
        {"AAPL", 1, 100.0, 10},
        {"MSFT", 2, 200.0, 20},
        {"NVDA", 3, 300.0, 30},
    };
    const QuerySpec query = parse_query_filters({"price>=150", "price<350"});

    const auto results = benchmark_strategies(events, query, {"linear_scan", "sorted_index"}, 1, 0);

    REQUIRE(results.size() == 2);
    REQUIRE(results[0].matches == 2);
    REQUIRE(results[1].matches == 2);
    REQUIRE(results[1].applicable);
}
