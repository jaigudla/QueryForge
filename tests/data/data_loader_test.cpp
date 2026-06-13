#include <catch2/catch_test_macros.hpp>

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
