#pragma once

#include "queryforge/data/table.hpp"

#include <string_view>
#include <vector>

TableSchema infer_table_schema(const std::vector<std::string>& headers,
                               const std::vector<std::vector<std::string>>& sample_rows);
TableSchema parse_table_schema(std::string_view schema);
CellValue parse_cell_value(const std::string& value,
                           ColumnType type,
                           const std::string& column_name,
                           std::size_t line_number);
