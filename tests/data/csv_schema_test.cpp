#include <catch2/catch_test_macros.hpp>

#include "queryforge/data/csv_schema.hpp"

#include <filesystem>
#include <stdexcept>

using namespace queryforge;

TEST_CASE("parse_table_schema parses arbitrary explicit schema", "[csv_schema]") {
    const TableSchema schema =
        parse_table_schema("user_id:int64,country:string,score:double,is_paid:bool");

    REQUIRE(schema.columns.size() == 4);
    REQUIRE(schema.columns[0].name == "user_id");
    REQUIRE(schema.columns[0].type == ColumnType::Int64);
    REQUIRE(schema.columns[1].type == ColumnType::String);
    REQUIRE(schema.columns[2].type == ColumnType::Double);
    REQUIRE(schema.columns[3].type == ColumnType::Bool);
}

TEST_CASE("parse_table_schema rejects unknown types and duplicate columns", "[csv_schema]") {
    REQUIRE_THROWS_AS(parse_table_schema("user_id:uuid"), std::runtime_error);
    REQUIRE_THROWS_AS(parse_table_schema("id:int64,id:string"), std::runtime_error);
}

TEST_CASE("parse_schema_file loads json schema", "[csv_schema]") {
    const std::filesystem::path path =
        std::filesystem::path(QUERYFORGE_SOURCE_DIR) / "examples/schemas/users.json";
    const TableSchema schema = parse_schema_file(path.string());

    REQUIRE(schema.columns.size() == 5);
    REQUIRE(schema.columns[0].name == "user_id");
    REQUIRE(schema.columns[4].type == ColumnType::Bool);
}

TEST_CASE("infer_table_schema infers common scalar types", "[csv_schema]") {
    const std::vector<std::string> headers = {"name", "age", "score", "is_paid"};
    const std::vector<std::vector<std::string>> rows = {
        {"Ada", "34", "91.2", "true"},
        {"Grace", "29", "88.0", "false"},
    };

    const TableSchema schema = infer_table_schema(headers, rows);

    REQUIRE(schema.columns[0].type == ColumnType::String);
    REQUIRE(schema.columns[1].type == ColumnType::Int64);
    REQUIRE(schema.columns[2].type == ColumnType::Double);
    REQUIRE(schema.columns[3].type == ColumnType::Bool);
}
