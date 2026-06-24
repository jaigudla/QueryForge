#include "queryforge/cli/inspect_command.hpp"

#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/data_loader.hpp"
#include "queryforge/data/table.hpp"

#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace queryforge {
namespace {

char parse_delimiter(const std::string& delimiter) {
    if (delimiter.size() != 1) {
        throw std::runtime_error("--delimiter must be exactly one character");
    }
    return delimiter[0];
}

std::string json_escape(const std::string& value) {
    std::ostringstream escaped;
    for (const char ch : value) {
        switch (ch) {
            case '"':
                escaped << "\\\"";
                break;
            case '\\':
                escaped << "\\\\";
                break;
            default:
                escaped << ch;
                break;
        }
    }
    return escaped.str();
}

Table load_inspect_table(const InspectOptions& options) {
    CsvLoadOptions load_options;
    load_options.delimiter = parse_delimiter(options.delimiter);
    load_options.infer_schema = options.infer_schema;
    if (!options.schema_file.empty()) {
        load_options.schema = parse_schema_file(options.schema_file);
    } else if (!options.schema.empty()) {
        load_options.schema = parse_table_schema(options.schema);
    }
    return load_csv_table(options.input_path, load_options);
}

}  // namespace

Result<void> execute_inspect(const InspectOptions& options) {
    try {
        if (options.input_path.empty()) {
            return Result<void>::fail("inspect requires an input CSV path");
        }

        const Table table = load_inspect_table(options);
        const std::size_t preview = std::min(options.preview_rows, table.rows.size());

        if (options.output == "json") {
            std::cout << "{\n";
            std::cout << std::format("  \"rows\": {},\n", table.rows.size());
            std::cout << "  \"columns\": [\n";
            for (std::size_t i = 0; i < table.schema.columns.size(); ++i) {
                const ColumnSchema& column = table.schema.columns[i];
                std::cout << "    {\"name\": \"" << json_escape(column.name) << "\", \"type\": \""
                          << json_escape(column_type_name(column.type)) << "\"}"
                          << (i + 1 == table.schema.columns.size() ? "\n" : ",\n");
            }
            std::cout << "  ],\n  \"preview\": [\n";
            for (std::size_t row = 0; row < preview; ++row) {
                std::cout << "    [";
                for (std::size_t column = 0; column < table.schema.columns.size(); ++column) {
                    std::cout << "\"" << json_escape(cell_to_string(table.rows[row][column])) << "\"";
                    if (column + 1 < table.schema.columns.size()) {
                        std::cout << ", ";
                    }
                }
                std::cout << "]" << (row + 1 == preview ? "\n" : ",\n");
            }
            std::cout << "  ]\n}\n";
            return Result<void>::ok();
        }

        std::cout << "QueryForge inspect\n\n";
        std::cout << std::format("Input: {}\n", options.input_path);
        std::cout << std::format("Rows: {}\n\n", table.rows.size());
        std::cout << "Schema:\n";
        for (const ColumnSchema& column : table.schema.columns) {
            std::cout << std::format("  {}:{}\n", column.name, column_type_name(column.type));
        }

        if (preview > 0) {
            std::cout << "\nPreview:\n";
            for (std::size_t row = 0; row < preview; ++row) {
                std::cout << "  ";
                for (std::size_t column = 0; column < table.schema.columns.size(); ++column) {
                    if (column > 0) {
                        std::cout << ", ";
                    }
                    std::cout << table.schema.columns[column].name << '='
                              << cell_to_string(table.rows[row][column]);
                }
                std::cout << '\n';
            }
        }

        return Result<void>::ok();
    } catch (const std::exception& ex) {
        return Result<void>::fail(ex.what());
    }
}

}  // namespace queryforge
