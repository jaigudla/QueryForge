#include "queryforge/query/query_filter.hpp"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace {

std::string trim(std::string_view value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return std::string{value.substr(start, end - start + 1)};
}

QueryField parse_field(const std::string& value) {
    if (value == "symbol") {
        return QueryField::Symbol;
    }
    if (value == "timestamp") {
        return QueryField::Timestamp;
    }
    if (value == "price") {
        return QueryField::Price;
    }
    if (value == "quantity") {
        return QueryField::Quantity;
    }
    throw std::runtime_error("Unknown query field: " + value);
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

template <typename T>
T parse_integer(const std::string& value, const std::string& field) {
    T parsed{};
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw std::runtime_error("Invalid numeric value for " + field + ": " + value);
    }
    return parsed;
}

double parse_double(const std::string& value, const std::string& field) {
    try {
        std::size_t parsed_chars = 0;
        const double parsed = std::stod(value, &parsed_chars);
        if (parsed_chars != value.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric value for " + field + ": " + value);
    }
}

bool compare_integer(int64_t lhs, QueryOperator op, int64_t rhs) {
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

bool compare_double(double lhs, QueryOperator op, double rhs) {
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

}  // namespace

std::string field_name(QueryField field) {
    switch (field) {
        case QueryField::Symbol:
            return "symbol";
        case QueryField::Timestamp:
            return "timestamp";
        case QueryField::Price:
            return "price";
        case QueryField::Quantity:
            return "quantity";
    }
    return "unknown";
}

QuerySpec parse_query_filters(const std::vector<std::string>& clauses) {
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

        const QueryField field = parse_field(field_text);
        if (field == QueryField::Symbol && op != QueryOperator::Equal) {
            throw std::runtime_error("symbol only supports equality filters");
        }

        QueryFilter filter{field, op, value_text};
        if (field == QueryField::Timestamp || field == QueryField::Quantity) {
            filter.integer_value = parse_integer<int64_t>(value_text, field_text);
        } else if (field == QueryField::Price) {
            filter.double_value = parse_double(value_text, field_text);
        }

        query.filters.push_back(filter);
    }

    if (query.filters.empty()) {
        throw std::runtime_error("At least one query filter is required");
    }

    return query;
}

bool matches_query(const TradeEvent& event, const QuerySpec& query) {
    return std::all_of(query.filters.begin(), query.filters.end(), [&](const QueryFilter& filter) {
        switch (filter.field) {
            case QueryField::Symbol:
                return event.symbol == filter.value_text;
            case QueryField::Timestamp:
                return compare_integer(event.timestamp, filter.op, filter.integer_value);
            case QueryField::Price:
                return compare_double(event.price, filter.op, filter.double_value);
            case QueryField::Quantity:
                return compare_integer(event.quantity, filter.op, filter.integer_value);
        }
        return false;
    });
}

std::string describe_query(const QuerySpec& query) {
    std::ostringstream output;
    for (std::size_t i = 0; i < query.filters.size(); ++i) {
        const QueryFilter& filter = query.filters[i];
        if (i > 0) {
            output << " AND ";
        }
        output << field_name(filter.field) << ' ' << operator_text(filter.op) << ' ';
        if (filter.field == QueryField::Symbol) {
            output << '"' << filter.value_text << '"';
        } else {
            output << filter.value_text;
        }
    }
    return output.str();
}

bool find_symbol_equality(const QuerySpec& query, std::string& symbol) {
    for (const QueryFilter& filter : query.filters) {
        if (filter.field == QueryField::Symbol && filter.op == QueryOperator::Equal) {
            symbol = filter.value_text;
            return true;
        }
    }
    return false;
}

bool has_range_filter(const QuerySpec& query, QueryField field) {
    for (const QueryFilter& filter : query.filters) {
        if (filter.field == field && filter.op != QueryOperator::Equal) {
            return true;
        }
    }
    return false;
}
