#include "queryforge/data/workload.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace queryforge {
namespace {

std::string trim(const std::string& value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::vector<std::string> parse_inline_list(const std::string& value) {
    std::vector<std::string> items;
    std::string current;
    bool in_brackets = false;
    for (char ch : value) {
        if (ch == '[') {
            in_brackets = true;
            continue;
        }
        if (ch == ']') {
            in_brackets = false;
            if (!current.empty()) {
                items.push_back(trim(current));
                current.clear();
            }
            continue;
        }
        if (in_brackets && ch == ',') {
            if (!current.empty()) {
                items.push_back(trim(current));
                current.clear();
            }
            continue;
        }
        if (in_brackets) {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        items.push_back(trim(current));
    }
    return items;
}

}  // namespace

WorkloadSpec parse_workload_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open workload file: " + path);
    }

    WorkloadSpec workload;
    WeightedQuery current_query;
    bool in_query = false;

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line == "queries:") {
            continue;
        }

        if (line.rfind("- where:", 0) == 0) {
            if (in_query) {
                workload.queries.push_back(current_query);
            }
            current_query = WeightedQuery{};
            current_query.where_clauses = parse_inline_list(line.substr(8));
            in_query = true;
            continue;
        }

        if (line.rfind("where:", 0) == 0) {
            if (in_query) {
                workload.queries.push_back(current_query);
            }
            current_query = WeightedQuery{};
            current_query.where_clauses = parse_inline_list(line.substr(6));
            in_query = true;
            continue;
        }

        if (line.rfind("repeat_count:", 0) == 0) {
            workload.repeat_count = static_cast<std::size_t>(std::stoull(trim(line.substr(13))));
            continue;
        }

        if (line.rfind("weight:", 0) == 0 && in_query) {
            current_query.weight = std::stod(trim(line.substr(7)));
            continue;
        }
    }

    if (in_query) {
        workload.queries.push_back(current_query);
    }

    if (workload.queries.empty()) {
        throw std::runtime_error("Workload file must define at least one query");
    }

    double total_weight = 0.0;
    for (const WeightedQuery& query : workload.queries) {
        if (query.where_clauses.empty()) {
            throw std::runtime_error("Workload query is missing where clauses");
        }
        total_weight += query.weight;
    }
    if (total_weight <= 0.0) {
        throw std::runtime_error("Workload query weights must sum to a positive value");
    }

    for (WeightedQuery& query : workload.queries) {
        query.weight /= total_weight;
    }

    return workload;
}

std::vector<QuerySpec> resolve_workload_queries(const WorkloadSpec& workload,
                                                const TableSchema& schema) {
    std::vector<QuerySpec> queries;
    queries.reserve(workload.queries.size());
    for (const WeightedQuery& weighted : workload.queries) {
        queries.push_back(parse_query_filters(weighted.where_clauses, schema));
    }
    return queries;
}

}  // namespace queryforge
