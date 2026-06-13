#include <catch2/catch_test_macros.hpp>

#include "queryforge/query/query_filter.hpp"

#include <stdexcept>

TEST_CASE("parse_query_filters handles symbol equality", "[query_filter]") {
    const Table table{
        TableSchema{{{"symbol", ColumnType::String}}},
        {{{std::string{"AAPL"}}}},
    };
    const QuerySpec query = parse_query_filters({"symbol=AAPL"}, table.schema);

    REQUIRE(query.filters.size() == 1);
    REQUIRE(matches_query(table, 0, query));
    REQUIRE(describe_query(query) == "symbol == \"AAPL\"");
}

TEST_CASE("parse_query_filters handles numeric ranges", "[query_filter]") {
    const Table table{
        TableSchema{{{"price", ColumnType::Double}, {"quantity", ColumnType::Int64}}},
        {
            {150.0, int64_t{20}},
            {250.0, int64_t{20}},
            {150.0, int64_t{5}},
        },
    };
    const QuerySpec query = parse_query_filters({"price>=100", "price<200", "quantity>10"},
                                                table.schema);

    REQUIRE(matches_query(table, 0, query));
    REQUIRE_FALSE(matches_query(table, 1, query));
    REQUIRE_FALSE(matches_query(table, 2, query));
}

TEST_CASE("parse_query_filters rejects unknown fields", "[query_filter]") {
    const TableSchema schema{{{"country", ColumnType::String}}};
    REQUIRE_THROWS_AS(parse_query_filters({"exchange=NASDAQ"}, schema), std::runtime_error);
}

TEST_CASE("parse_query_filters handles arbitrary bool fields", "[query_filter]") {
    const Table table{
        TableSchema{{{"is_paid", ColumnType::Bool}}},
        {{{true}}, {{false}}},
    };
    const QuerySpec query = parse_query_filters({"is_paid=true"}, table.schema);

    REQUIRE(matches_query(table, 0, query));
    REQUIRE_FALSE(matches_query(table, 1, query));
}
