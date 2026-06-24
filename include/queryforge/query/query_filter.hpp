#pragma once

#include "queryforge/core/trade_event.hpp"
#include "queryforge/data/table.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace queryforge {

enum class QueryOperator {
    Equal,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
};

struct QueryFilter {
    std::string column;
    std::size_t column_index;
    ColumnType type;
    QueryOperator op;
    std::string value_text;
    CellValue value;
};

struct QuerySpec {
    std::vector<QueryFilter> filters;
};

QuerySpec parse_query_filters(const std::vector<std::string>& clauses, const TableSchema& schema);
bool matches_query(const Table& table, std::size_t row_index, const QuerySpec& query);
bool matches_query(const TradeEvent& event, const QuerySpec& query);
std::string describe_query(const QuerySpec& query);
bool find_equality_filter(const QuerySpec& query, QueryFilter& filter);
bool find_all_equality_filters(const QuerySpec& query, std::vector<QueryFilter>& filters);
bool find_range_filter(const QuerySpec& query, QueryFilter& filter);
bool find_best_range_filter(const QuerySpec& query, QueryFilter& filter);

}  // namespace queryforge
