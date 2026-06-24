#include <catch2/catch_test_macros.hpp>

#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/data_loader.hpp"
#include "queryforge/query/strategy.hpp"

#include <filesystem>
#include <string>

using namespace queryforge;

TEST_CASE("json regression fixture keeps stable match counts", "[regression]") {
    const std::filesystem::path fixture =
        std::filesystem::path(QUERYFORGE_SOURCE_DIR) / "benchmarks/fixtures/small_table.csv";

    const Table table = load_csv_table(fixture.string());
    const QuerySpec query = parse_query_filters({"country=US"}, table.schema);
    const auto results = benchmark_strategies(table, query, {"linear_scan", "sorted_index"}, 1, 0);

    REQUIRE(results[0].matches == 2);
    REQUIRE(results[0].applicable);
}

TEST_CASE("quoted fixture loads country with embedded comma", "[regression]") {
    const std::filesystem::path fixture =
        std::filesystem::path(QUERYFORGE_SOURCE_DIR) / "benchmarks/fixtures/small_table.csv";
    const Table table = load_csv_table(fixture.string());

    REQUIRE(std::get<std::string>(table.rows[1][0]) == "CA,East");
}
