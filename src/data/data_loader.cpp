#include "queryforge/data/data_loader.hpp"

#include "queryforge/data/csv_line_parser.hpp"
#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/dataset_generator.hpp"

#include <algorithm>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace queryforge {
namespace {

std::string trim(std::string_view value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return std::string{value.substr(start, end - start + 1)};
}

std::vector<std::string> split_and_trim_fields(const std::string& line, char delimiter) {
    std::vector<std::string> fields = parse_csv_line(line, delimiter);
    for (std::string& field : fields) {
        field = trim(field);
    }
    return fields;
}

void validate_schema_matches_header(const TableSchema& schema,
                                    const std::vector<std::string>& header) {
    for (const ColumnSchema& column : schema.columns) {
        if (std::find(header.begin(), header.end(), column.name) == header.end()) {
            throw std::runtime_error("CSV header is missing schema column: " + column.name);
        }
    }
}

}  // namespace

Table load_csv_table(const std::string& path, const CsvLoadOptions& options) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open CSV input: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("CSV input is empty: " + path);
    }

    const std::vector<std::string> header = split_and_trim_fields(line, options.delimiter);

    std::vector<std::vector<std::string>> raw_rows;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        ++line_number;
        if (trim(line).empty()) {
            continue;
        }

        const std::vector<std::string> fields = split_and_trim_fields(line, options.delimiter);
        if (fields.size() != header.size()) {
            throw std::runtime_error("CSV line " + std::to_string(line_number) + " has " +
                                     std::to_string(fields.size()) + " fields, expected " +
                                     std::to_string(header.size()));
        }
        raw_rows.push_back(fields);
    }

    if (raw_rows.empty()) {
        throw std::runtime_error("CSV input has no data rows: " + path);
    }

    Table table;
    if (options.schema.has_value()) {
        table.schema = *options.schema;
        validate_schema_matches_header(table.schema, header);
    } else if (options.infer_schema) {
        table.schema = infer_table_schema(header, raw_rows);
    } else {
        throw std::runtime_error("CSV loading requires schema inference or an explicit schema");
    }

    std::vector<std::size_t> source_indexes;
    source_indexes.reserve(table.schema.columns.size());
    for (const ColumnSchema& column : table.schema.columns) {
        const auto found = std::find(header.begin(), header.end(), column.name);
        if (found == header.end()) {
            throw std::runtime_error("CSV header is missing schema column: " + column.name);
        }
        source_indexes.push_back(static_cast<std::size_t>(std::distance(header.begin(), found)));
    }

    table.rows.reserve(raw_rows.size());
    for (std::size_t row_index = 0; row_index < raw_rows.size(); ++row_index) {
        std::vector<CellValue> row;
        row.reserve(table.schema.columns.size());
        for (std::size_t column = 0; column < table.schema.columns.size(); ++column) {
            const ColumnSchema& column_schema = table.schema.columns[column];
            row.push_back(parse_cell_value(raw_rows[row_index][source_indexes[column]],
                                           column_schema.type,
                                           column_schema.name,
                                           row_index + 2,
                                           source_indexes[column]));
        }
        table.rows.push_back(std::move(row));
    }

    return table;
}

std::vector<TradeEvent> load_trade_events_csv(const std::string& path) {
    CsvLoadOptions options;
    options.schema = default_trade_schema();
    const Table table = load_csv_table(path, options);
    const std::size_t symbol = column_index(table.schema, "symbol");
    const std::size_t timestamp = column_index(table.schema, "timestamp");
    const std::size_t price = column_index(table.schema, "price");
    const std::size_t quantity = column_index(table.schema, "quantity");

    std::vector<TradeEvent> events;
    events.reserve(table.rows.size());
    for (const std::vector<CellValue>& row : table.rows) {
        events.push_back(TradeEvent{
            std::get<std::string>(row[symbol]),
            std::get<int64_t>(row[timestamp]),
            std::get<double>(row[price]),
            static_cast<int>(std::get<int64_t>(row[quantity])),
        });
    }
    return events;
}

}  // namespace queryforge
