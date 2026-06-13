#include <catch2/catch_test_macros.hpp>

#include "queryforge/query/query_filter.hpp"

#include <stdexcept>

TEST_CASE("parse_query_filters handles symbol equality", "[query_filter]") {
    const QuerySpec query = parse_query_filters({"symbol=AAPL"});
    const TradeEvent event{"AAPL", 10, 100.0, 5};

    REQUIRE(query.filters.size() == 1);
    REQUIRE(matches_query(event, query));
    REQUIRE(describe_query(query) == "symbol == \"AAPL\"");
}

TEST_CASE("parse_query_filters handles numeric ranges", "[query_filter]") {
    const QuerySpec query = parse_query_filters({"price>=100", "price<200", "quantity>10"});

    REQUIRE(matches_query(TradeEvent{"AAPL", 10, 150.0, 20}, query));
    REQUIRE_FALSE(matches_query(TradeEvent{"AAPL", 10, 250.0, 20}, query));
    REQUIRE_FALSE(matches_query(TradeEvent{"AAPL", 10, 150.0, 5}, query));
}

TEST_CASE("parse_query_filters rejects unknown fields", "[query_filter]") {
    REQUIRE_THROWS_AS(parse_query_filters({"exchange=NASDAQ"}), std::runtime_error);
}
