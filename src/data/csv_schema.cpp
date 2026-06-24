#include "queryforge/data/csv_schema.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

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

std::vector<std::string> split(std::string_view value, char delimiter) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t end = value.find(delimiter, start);
        if (end == std::string_view::npos) {
            parts.push_back(trim(value.substr(start)));
            break;
        }
        parts.push_back(trim(value.substr(start, end - start)));
        start = end + 1;
    }
    return parts;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parses_int64(const std::string& value) {
    int64_t parsed = 0;
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parses_double(const std::string& value) {
    try {
        std::size_t parsed_chars = 0;
        (void)std::stod(value, &parsed_chars);
        return parsed_chars == value.size();
    } catch (const std::exception&) {
        return false;
    }
}

bool parses_bool(const std::string& value) {
    const std::string normalized = lower(value);
    return normalized == "true" || normalized == "false" || normalized == "1" ||
           normalized == "0" || normalized == "yes" || normalized == "no";
}

void validate_unique_columns(const TableSchema& schema) {
    std::set<std::string> seen;
    for (const ColumnSchema& column : schema.columns) {
        if (column.name.empty()) {
            throw std::runtime_error("CSV schema contains an empty column name");
        }
        if (!seen.insert(column.name).second) {
            throw std::runtime_error("CSV schema contains duplicate column: " + column.name);
        }
    }
}

std::string extract_json_string(const std::string& json, std::size_t& pos) {
    const std::size_t start = json.find('"', pos);
    if (start == std::string::npos) {
        throw std::runtime_error("Malformed schema JSON: expected string");
    }
    std::ostringstream value;
    for (std::size_t i = start + 1; i < json.size(); ++i) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            value << json[++i];
            continue;
        }
        if (json[i] == '"') {
            pos = i + 1;
            return value.str();
        }
        value << json[i];
    }
    throw std::runtime_error("Malformed schema JSON: unterminated string");
}

TableSchema parse_json_schema(const std::string& json) {
    TableSchema schema;
    const std::string columns_key = "\"columns\"";
    const std::size_t columns_pos = json.find(columns_key);
    if (columns_pos == std::string::npos) {
        throw std::runtime_error("Schema JSON must contain a columns array");
    }

    std::size_t pos = json.find('[', columns_pos);
    if (pos == std::string::npos) {
        throw std::runtime_error("Schema JSON must contain a columns array");
    }
    ++pos;

    while (pos < json.size()) {
        pos = json.find('{', pos);
        if (pos == std::string::npos) {
            break;
        }
        ++pos;

        std::string name;
        std::string type;
        while (pos < json.size() && json[pos] != '}') {
            if (json[pos] == '"') {
                const std::string key = extract_json_string(json, pos);
                pos = json.find(':', pos);
                if (pos == std::string::npos) {
                    break;
                }
                ++pos;
                while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
                    ++pos;
                }
                if (json[pos] != '"') {
                    throw std::runtime_error("Schema JSON column values must be strings");
                }
                const std::string value = extract_json_string(json, pos);
                if (key == "name") {
                    name = value;
                } else if (key == "type") {
                    type = value;
                }
            } else {
                ++pos;
            }
        }

        if (!name.empty() && !type.empty()) {
            schema.columns.push_back(ColumnSchema{name, parse_column_type(type)});
        }
        pos = json.find('}', pos);
        if (pos == std::string::npos) {
            break;
        }
        ++pos;
    }

    if (schema.columns.empty()) {
        throw std::runtime_error("Schema JSON must contain at least one column");
    }

    validate_unique_columns(schema);
    return schema;
}

}  // namespace

TableSchema parse_table_schema(std::string_view schema_text) {
    TableSchema schema;
    for (const std::string& entry : split(schema_text, ',')) {
        if (entry.empty()) {
            continue;
        }

        const std::size_t colon = entry.find(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("Schema entry must use name:type syntax: " + entry);
        }

        const std::string name = trim(std::string_view{entry}.substr(0, colon));
        const std::string type = trim(std::string_view{entry}.substr(colon + 1));
        if (name.empty() || type.empty()) {
            throw std::runtime_error("Schema entry must use name:type syntax: " + entry);
        }
        schema.columns.push_back(ColumnSchema{name, parse_column_type(type)});
    }

    if (schema.columns.empty()) {
        throw std::runtime_error("CSV schema must contain at least one column");
    }

    validate_unique_columns(schema);
    return schema;
}

TableSchema parse_schema_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open schema file: " + path);
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    const std::string text = contents.str();
    const std::string trimmed = trim(text);
    if (trimmed.empty()) {
        throw std::runtime_error("Schema file is empty: " + path);
    }

    if (trimmed.front() == '{') {
        return parse_json_schema(trimmed);
    }

    std::vector<std::string> lines;
    std::istringstream stream(trimmed);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (!line.empty() && line[0] != '#') {
            lines.push_back(line);
        }
    }

    if (lines.empty()) {
        throw std::runtime_error("Schema file has no column definitions: " + path);
    }

    std::ostringstream joined;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            joined << ',';
        }
        joined << lines[i];
    }
    return parse_table_schema(joined.str());
}

TableSchema infer_table_schema(const std::vector<std::string>& headers,
                               const std::vector<std::vector<std::string>>& sample_rows) {
    TableSchema schema;
    schema.columns.reserve(headers.size());

    for (std::size_t column = 0; column < headers.size(); ++column) {
        bool can_bool = true;
        bool can_int = true;
        bool can_double = true;

        for (const std::vector<std::string>& row : sample_rows) {
            if (column >= row.size() || row[column].empty()) {
                continue;
            }

            can_bool = can_bool && parses_bool(row[column]);
            can_int = can_int && parses_int64(row[column]);
            can_double = can_double && parses_double(row[column]);
        }

        ColumnType type = ColumnType::String;
        if (can_bool) {
            type = ColumnType::Bool;
        } else if (can_int) {
            type = ColumnType::Int64;
        } else if (can_double) {
            type = ColumnType::Double;
        }

        schema.columns.push_back(ColumnSchema{headers[column], type});
    }

    validate_unique_columns(schema);
    return schema;
}

CellValue parse_cell_value(const std::string& value,
                           ColumnType type,
                           const std::string& column_name,
                           std::size_t line_number,
                           std::size_t column_number) {
    const std::string location = "line " + std::to_string(line_number) + ", column " +
                                 std::to_string(column_number + 1) + " ('" + column_name + "')";

    switch (type) {
        case ColumnType::String:
            return value;
        case ColumnType::Int64: {
            int64_t parsed = 0;
            const char* begin = value.data();
            const char* end = value.data() + value.size();
            const auto result = std::from_chars(begin, end, parsed);
            if (result.ec != std::errc{} || result.ptr != end) {
                throw std::runtime_error("Expected int64 at " + location + ", got \"" + value + "\"");
            }
            return parsed;
        }
        case ColumnType::Double: {
            try {
                std::size_t parsed_chars = 0;
                const double parsed = std::stod(value, &parsed_chars);
                if (parsed_chars != value.size()) {
                    throw std::invalid_argument("trailing characters");
                }
                return parsed;
            } catch (const std::exception&) {
                throw std::runtime_error("Expected double at " + location + ", got \"" + value + "\"");
            }
        }
        case ColumnType::Bool: {
            const std::string normalized = lower(value);
            if (normalized == "true" || normalized == "1" || normalized == "yes") {
                return true;
            }
            if (normalized == "false" || normalized == "0" || normalized == "no") {
                return false;
            }
            throw std::runtime_error("Expected bool at " + location + ", got \"" + value + "\"");
        }
    }
    throw std::runtime_error("Unsupported column type");
}

}  // namespace queryforge
