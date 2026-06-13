#pragma once

#include "queryforge/core/trade_event.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

enum class ColumnType {
    String,
    Int64,
    Double,
    Bool,
};

using CellValue = std::variant<std::string, int64_t, double, bool>;

struct ColumnSchema {
    std::string name;
    ColumnType type;
};

struct TableSchema {
    std::vector<ColumnSchema> columns;
};

struct Table {
    TableSchema schema;
    std::vector<std::vector<CellValue>> rows;
};

std::string column_type_name(ColumnType type);
ColumnType parse_column_type(const std::string& type);
std::size_t column_index(const TableSchema& schema, const std::string& name);
std::string cell_to_string(const CellValue& value);
double cell_to_number(const CellValue& value);
Table trade_events_to_table(const std::vector<TradeEvent>& events);
