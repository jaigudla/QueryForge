#include <catch2/catch_test_macros.hpp>

#include "queryforge/data/csv_line_parser.hpp"

using namespace queryforge;

TEST_CASE("parse_csv_line handles quoted commas", "[csv_line_parser]") {
    const auto fields = parse_csv_line(R"("CA,East",29)", ',');
    REQUIRE(fields.size() == 2);
    REQUIRE(fields[0] == "CA,East");
    REQUIRE(fields[1] == "29");
}

TEST_CASE("parse_csv_line handles escaped quotes", "[csv_line_parser]") {
    const auto fields = parse_csv_line(R"("say ""hi""",done)", ',');
    REQUIRE(fields.size() == 2);
    REQUIRE(fields[0] == "say \"hi\"");
    REQUIRE(fields[1] == "done");
}

TEST_CASE("parse_csv_line rejects unclosed quotes", "[csv_line_parser]") {
    REQUIRE_THROWS_AS(parse_csv_line("\"open", ','), std::runtime_error);
}
