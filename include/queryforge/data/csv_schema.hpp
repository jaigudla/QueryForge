#pragma once

#include "queryforge/data/table.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace queryforge {

TableSchema infer_table_schema(const std::vector<std::string>& headers,
                               const std::vector<std::vector<std::string>>& sample_rows);
TableSchema parse_table_schema(std::string_view schema);
TableSchema parse_schema_file(const std::string& path);
CellValue parse_cell_value(const std::string& value,
                           ColumnType type,
                           const std::string& column_name,
                           std::size_t line_number,
                           std::size_t column_number = 0);

}  // namespace queryforge
