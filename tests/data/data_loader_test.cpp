#include <catch2/catch_test_macros.hpp>

#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/data_loader.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

std::filesystem::path write_temp_csv(const std::string& contents) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "queryforge_data_loader_test.csv";
    std::ofstream output(path);
    output << contents;
    return path;
}

}  // namespace

TEST_CASE("load_trade_events_csv parses valid trades", "[data_loader]") {
    const std::filesystem::path path = write_temp_csv(
        "symbol,timestamp,price,quantity\n"
        "AAPL,1600000000000000000,185.12,100\n"
        "MSFT,1600000000000000100,410.50,80\n");

    const auto events = load_trade_events_csv(path.string());

    REQUIRE(events.size() == 2);
    REQUIRE(events[0].symbol == "AAPL");
    REQUIRE(events[0].timestamp == 1600000000000000000LL);
    REQUIRE(events[0].price == 185.12);
    REQUIRE(events[0].quantity == 100);
}

TEST_CASE("load_trade_events_csv rejects missing required columns", "[data_loader]") {
    const std::filesystem::path path = write_temp_csv(
        "symbol,timestamp,price\n"
        "AAPL,1600000000000000000,185.12\n");

    REQUIRE_THROWS_AS(load_trade_events_csv(path.string()), std::runtime_error);
}

TEST_CASE("load_trade_events_csv rejects invalid rows", "[data_loader]") {
    const std::filesystem::path path = write_temp_csv(
        "symbol,timestamp,price,quantity\n"
        "AAPL,not-a-time,185.12,100\n");

    REQUIRE_THROWS_AS(load_trade_events_csv(path.string()), std::runtime_error);
}

TEST_CASE("load_csv_table loads arbitrary inferred schemas", "[data_loader]") {
    const std::filesystem::path path = write_temp_csv(
        "user_id,country,age,score,is_paid\n"
        "1,US,34,91.2,true\n"
        "2,CA,29,77.0,false\n");

    const Table table = load_csv_table(path.string());

    REQUIRE(table.schema.columns.size() == 5);
    REQUIRE(table.schema.columns[0].name == "user_id");
    REQUIRE(table.schema.columns[0].type == ColumnType::Int64);
    REQUIRE(table.schema.columns[1].type == ColumnType::String);
    REQUIRE(table.schema.columns[4].type == ColumnType::Bool);
    REQUIRE(table.rows.size() == 2);
    REQUIRE(std::get<std::string>(table.rows[0][1]) == "US");
    REQUIRE(std::get<int64_t>(table.rows[0][2]) == 34);
    REQUIRE(std::get<bool>(table.rows[0][4]));
}

TEST_CASE("load_csv_table uses explicit schema and delimiter", "[data_loader]") {
    const std::filesystem::path path = write_temp_csv(
        "event_id|level|frame_time_ms|entity_count\n"
        "1001|info|16.4|1200\n"
        "1002|warn|33.8|1400\n");

    CsvLoadOptions options;
    options.delimiter = '|';
    options.schema = parse_table_schema(
        "event_id:int64,level:string,frame_time_ms:double,entity_count:int64");

    const Table table = load_csv_table(path.string(), options);

    REQUIRE(table.rows.size() == 2);
    REQUIRE(std::get<double>(table.rows[1][2]) == 33.8);
}

TEST_CASE("load_csv_table rejects inconsistent row widths", "[data_loader]") {
    const std::filesystem::path path = write_temp_csv(
        "id,country,age\n"
        "1,US,34\n"
        "2,CA\n");

    REQUIRE_THROWS_AS(load_csv_table(path.string()), std::runtime_error);
}
