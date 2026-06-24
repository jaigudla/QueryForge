#include <catch2/catch_test_macros.hpp>

#include "queryforge/query/strategy.hpp"

using namespace queryforge;

TEST_CASE("benchmark_strategies compares linear scan and hash index", "[strategy]") {
    const Table table{
        TableSchema{{{"country", ColumnType::String}, {"age", ColumnType::Int64}}},
        {
            {std::string{"US"}, int64_t{34}},
            {std::string{"CA"}, int64_t{29}},
            {std::string{"US"}, int64_t{41}},
            {std::string{"GB"}, int64_t{37}},
        },
    };
    const QuerySpec query = parse_query_filters({"country=US"}, table.schema);

    const auto results = benchmark_strategies(table, query, {"linear_scan", "hash_index"}, 1, 0);

    REQUIRE(results.size() == 2);
    REQUIRE(results[0].matches == 2);
    REQUIRE(results[1].matches == 2);
    REQUIRE(results[1].applicable);
}

TEST_CASE("benchmark_strategies marks unsupported sorted index as not applicable", "[strategy]") {
    const Table table{
        TableSchema{{{"country", ColumnType::String}}},
        {
            {std::string{"US"}},
            {std::string{"CA"}},
        },
    };
    const QuerySpec query = parse_query_filters({"country=US"}, table.schema);

    const auto results = benchmark_strategies(table, query, {"sorted_index"}, 1, 0);

    REQUIRE(results.size() == 1);
    REQUIRE_FALSE(results[0].applicable);
}

TEST_CASE("benchmark_strategies supports sorted index for range filters", "[strategy]") {
    const Table table{
        TableSchema{{{"frame_time_ms", ColumnType::Double}}},
        {
            {100.0},
            {200.0},
            {300.0},
        },
    };
    const QuerySpec query = parse_query_filters({"frame_time_ms>=150", "frame_time_ms<350"},
                                                table.schema);

    const auto results = benchmark_strategies(table, query, {"linear_scan", "sorted_index"}, 1, 0);

    REQUIRE(results.size() == 2);
    REQUIRE(results[0].matches == 2);
    REQUIRE(results[1].matches == 2);
    REQUIRE(results[1].applicable);
}
