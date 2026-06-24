#include <catch2/catch_test_macros.hpp>

#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/dataset_generator.hpp"
#include "queryforge/query/linear_scan.hpp"

#include <unordered_set>

using namespace queryforge;

TEST_CASE("generate_dataset produces requested row count", "[dataset_generator]") {
    const auto events = generate_dataset(10'000);
    REQUIRE(events.size() == 10'000);
}

TEST_CASE("generate_dataset uses symbols from the pool", "[dataset_generator]") {
    const auto pool = build_symbol_pool();
    const std::unordered_set<std::string> pool_set(pool.begin(), pool.end());

    const auto events = generate_dataset(10'000);
    for (const TradeEvent& event : events) {
        REQUIRE(pool_set.contains(event.symbol));
    }
}

TEST_CASE("generate_dataset is reproducible for a fixed seed", "[dataset_generator]") {
    const auto first = generate_dataset(10'000, 42);
    const auto second = generate_dataset(10'000, 42);

    const ScanResult first_result = linear_scan_benchmark(first, "AAPL", 1, 0);
    const ScanResult second_result = linear_scan_benchmark(second, "AAPL", 1, 0);

    REQUIRE(first_result.matches == second_result.matches);
}

TEST_CASE("generate_table produces schema-shaped rows", "[dataset_generator]") {
    const TableSchema schema = parse_table_schema("user_id:int64,country:string");
    const Table table = generate_table(schema, 100, 42);

    REQUIRE(table.rows.size() == 100);
    REQUIRE(std::holds_alternative<int64_t>(table.rows[0][0]));
    REQUIRE(std::holds_alternative<std::string>(table.rows[0][1]));
}
