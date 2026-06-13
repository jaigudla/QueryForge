#pragma once

#include "queryforge/core/trade_event.hpp"

#include <cstdint>
#include <string>
#include <vector>

enum class QueryField {
    Symbol,
    Timestamp,
    Price,
    Quantity,
};

enum class QueryOperator {
    Equal,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
};

struct QueryFilter {
    QueryField field;
    QueryOperator op;
    std::string value_text;
    int64_t integer_value = 0;
    double double_value = 0.0;
};

struct QuerySpec {
    std::vector<QueryFilter> filters;
};

QuerySpec parse_query_filters(const std::vector<std::string>& clauses);
bool matches_query(const TradeEvent& event, const QuerySpec& query);
std::string describe_query(const QuerySpec& query);
bool find_symbol_equality(const QuerySpec& query, std::string& symbol);
bool has_range_filter(const QuerySpec& query, QueryField field);
std::string field_name(QueryField field);
