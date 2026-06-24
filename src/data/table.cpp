#include "queryforge/data/dataset_generator.hpp"
#include "queryforge/data/table.hpp"

#include <format>
#include <stdexcept>
#include <type_traits>

namespace queryforge {

std::string column_type_name(ColumnType type) {
    switch (type) {
        case ColumnType::String:
            return "string";
        case ColumnType::Int64:
            return "int64";
        case ColumnType::Double:
            return "double";
        case ColumnType::Bool:
            return "bool";
    }
    return "unknown";
}

ColumnType parse_column_type(const std::string& type) {
    if (type == "string") {
        return ColumnType::String;
    }
    if (type == "int64" || type == "int") {
        return ColumnType::Int64;
    }
    if (type == "double" || type == "float") {
        return ColumnType::Double;
    }
    if (type == "bool" || type == "boolean") {
        return ColumnType::Bool;
    }
    throw std::runtime_error("Unknown column type: " + type);
}

std::size_t column_index(const TableSchema& schema, const std::string& name) {
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        if (schema.columns[i].name == name) {
            return i;
        }
    }
    throw std::runtime_error("Unknown column: " + name);
}

std::string cell_to_string(const CellValue& value) {
    return std::visit(
        [](const auto& current) -> std::string {
            using T = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return current;
            } else if constexpr (std::is_same_v<T, bool>) {
                return current ? "true" : "false";
            } else {
                return std::format("{}", current);
            }
        },
        value);
}

double cell_to_number(const CellValue& value) {
    if (const auto* integer = std::get_if<int64_t>(&value)) {
        return static_cast<double>(*integer);
    }
    if (const auto* floating = std::get_if<double>(&value)) {
        return *floating;
    }
    throw std::runtime_error("Column value is not numeric");
}

Table trade_events_to_table(const std::vector<TradeEvent>& events) {
    Table table{default_trade_schema(), {}};
    table.rows.reserve(events.size());

    for (const TradeEvent& event : events) {
        table.rows.push_back({
            event.symbol,
            event.timestamp,
            event.price,
            static_cast<int64_t>(event.quantity),
        });
    }

    return table;
}

}  // namespace queryforge
