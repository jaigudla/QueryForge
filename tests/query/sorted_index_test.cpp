#include <catch2/catch_test_macros.hpp>

#include "queryforge/query/strategy.hpp"

using namespace queryforge;

TEST_CASE("sorted_index matches linear_scan on range queries", "[sorted_index]") {
    Table table{
        TableSchema{{{"value", ColumnType::Int64}}},
        {
            {int64_t{10}},
            {int64_t{20}},
            {int64_t{30}},
            {int64_t{40}},
            {int64_t{50}},
        },
    };
    const QuerySpec query = parse_query_filters({"value>=25", "value<=45"}, table.schema);

    const auto linear = benchmark_strategies(table, query, {"linear_scan"}, 1, 0);
    const auto sorted = benchmark_strategies(table, query, {"sorted_index"}, 1, 0);

    REQUIRE(linear[0].matches == 2);
    REQUIRE(sorted[0].matches == 2);
    REQUIRE(sorted[0].applicable);
}

TEST_CASE("sorted_index handles open-ended ranges", "[sorted_index]") {
    Table table{
        TableSchema{{{"price", ColumnType::Double}}},
        {
            {10.0},
            {50.0},
            {90.0},
        },
    };
    const QuerySpec query = parse_query_filters({"price>40"}, table.schema);

    const auto linear = benchmark_strategies(table, query, {"linear_scan"}, 1, 0);
    const auto sorted = benchmark_strategies(table, query, {"sorted_index"}, 1, 0);

    REQUIRE(linear[0].matches == 2);
    REQUIRE(sorted[0].matches == 2);
}
