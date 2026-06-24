#include "queryforge/query/query_filter.hpp"

#include "queryforge/data/csv_schema.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
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

std::string operator_text(QueryOperator op) {
    switch (op) {
        case QueryOperator::Equal:
            return "==";
        case QueryOperator::Less:
            return "<";
        case QueryOperator::LessEqual:
            return "<=";
        case QueryOperator::Greater:
            return ">";
        case QueryOperator::GreaterEqual:
            return ">=";
    }
    return "?";
}

bool compare_number(double lhs, QueryOperator op, double rhs) {
    switch (op) {
        case QueryOperator::Equal:
            return lhs == rhs;
        case QueryOperator::Less:
            return lhs < rhs;
        case QueryOperator::LessEqual:
            return lhs <= rhs;
        case QueryOperator::Greater:
            return lhs > rhs;
        case QueryOperator::GreaterEqual:
            return lhs >= rhs;
    }
    return false;
}

bool compare_string(const std::string& lhs, QueryOperator op, const std::string& rhs) {
    switch (op) {
        case QueryOperator::Equal:
            return lhs == rhs;
        case QueryOperator::Less:
            return lhs < rhs;
        case QueryOperator::LessEqual:
            return lhs <= rhs;
        case QueryOperator::Greater:
            return lhs > rhs;
        case QueryOperator::GreaterEqual:
            return lhs >= rhs;
    }
    return false;
}

bool compare_bool(bool lhs, QueryOperator op, bool rhs) {
    if (op != QueryOperator::Equal) {
        throw std::runtime_error("bool columns only support equality filters");
    }
    return lhs == rhs;
}

bool compare_cell(const CellValue& lhs, QueryOperator op, const CellValue& rhs) {
    if (const auto* left = std::get_if<std::string>(&lhs)) {
        return compare_string(*left, op, std::get<std::string>(rhs));
    }
    if (const auto* left = std::get_if<int64_t>(&lhs)) {
        return compare_number(static_cast<double>(*left), op, static_cast<double>(std::get<int64_t>(rhs)));
    }
    if (const auto* left = std::get_if<double>(&lhs)) {
        return compare_number(*left, op, std::get<double>(rhs));
    }
    if (const auto* left = std::get_if<bool>(&lhs)) {
        return compare_bool(*left, op, std::get<bool>(rhs));
    }
    return false;
}

}  // namespace

QuerySpec parse_query_filters(const std::vector<std::string>& clauses, const TableSchema& schema) {
    QuerySpec query;
    for (const std::string& raw_clause : clauses) {
        const std::string clause = trim(raw_clause);
        if (clause.empty()) {
            continue;
        }

        QueryOperator op = QueryOperator::Equal;
        std::size_t op_pos = std::string::npos;
        std::size_t op_len = 0;
        for (const auto& candidate : {std::string(">="), std::string("<="), std::string("="),
                                      std::string(">"), std::string("<")}) {
            op_pos = clause.find(candidate);
            if (op_pos != std::string::npos) {
                op_len = candidate.size();
                if (candidate == ">=") {
                    op = QueryOperator::GreaterEqual;
                } else if (candidate == "<=") {
                    op = QueryOperator::LessEqual;
                } else if (candidate == ">") {
                    op = QueryOperator::Greater;
                } else if (candidate == "<") {
                    op = QueryOperator::Less;
                } else {
                    op = QueryOperator::Equal;
                }
                break;
            }
        }

        if (op_pos == std::string::npos) {
            throw std::runtime_error("Malformed query filter: " + clause);
        }

        const std::string field_text = trim(std::string_view{clause}.substr(0, op_pos));
        const std::string value_text = trim(std::string_view{clause}.substr(op_pos + op_len));
        if (field_text.empty() || value_text.empty()) {
            throw std::runtime_error("Malformed query filter: " + clause);
        }

        const std::size_t index = column_index(schema, field_text);
        const ColumnType type = schema.columns[index].type;
        if (type == ColumnType::Bool && op != QueryOperator::Equal) {
            throw std::runtime_error("bool column '" + field_text + "' only supports equality filters");
        }

        QueryFilter filter{
            field_text,
            index,
            type,
            op,
            value_text,
            parse_cell_value(value_text, type, field_text, 0),
        };
        query.filters.push_back(filter);
    }

    if (query.filters.empty()) {
        throw std::runtime_error("At least one query filter is required");
    }

    return query;
}

bool matches_query(const Table& table, std::size_t row_index, const QuerySpec& query) {
    const std::vector<CellValue>& row = table.rows.at(row_index);
    return std::all_of(query.filters.begin(), query.filters.end(), [&](const QueryFilter& filter) {
        return compare_cell(row.at(filter.column_index), filter.op, filter.value);
    });
}

bool matches_query(const TradeEvent& event, const QuerySpec& query) {
    const Table table = trade_events_to_table({event});
    return matches_query(table, 0, query);
}

std::string describe_query(const QuerySpec& query) {
    std::ostringstream output;
    for (std::size_t i = 0; i < query.filters.size(); ++i) {
        const QueryFilter& filter = query.filters[i];
        if (i > 0) {
            output << " AND ";
        }
        output << filter.column << ' ' << operator_text(filter.op) << ' ';
        output << (filter.type == ColumnType::String ? "\"" + filter.value_text + "\"" : filter.value_text);
    }
    return output.str();
}

bool find_equality_filter(const QuerySpec& query, QueryFilter& filter) {
    for (const QueryFilter& candidate : query.filters) {
        if (candidate.op == QueryOperator::Equal) {
            filter = candidate;
            return true;
        }
    }
    return false;
}

bool find_all_equality_filters(const QuerySpec& query, std::vector<QueryFilter>& filters) {
    filters.clear();
    for (const QueryFilter& candidate : query.filters) {
        if (candidate.op == QueryOperator::Equal) {
            filters.push_back(candidate);
        }
    }
    return !filters.empty();
}

bool find_range_filter(const QuerySpec& query, QueryFilter& filter) {
    for (const QueryFilter& candidate : query.filters) {
        if ((candidate.type == ColumnType::Int64 || candidate.type == ColumnType::Double) &&
            candidate.op != QueryOperator::Equal) {
            filter = candidate;
            return true;
        }
    }
    return false;
}

bool find_best_range_filter(const QuerySpec& query, QueryFilter& filter) {
    bool found = false;
    for (const QueryFilter& candidate : query.filters) {
        if ((candidate.type == ColumnType::Int64 || candidate.type == ColumnType::Double) &&
            candidate.op != QueryOperator::Equal) {
            if (!found) {
                filter = candidate;
                found = true;
            } else if (candidate.op == QueryOperator::Equal) {
                continue;
            } else {
                filter = candidate;
            }
        }
    }
    return found;
}

}  // namespace queryforge
