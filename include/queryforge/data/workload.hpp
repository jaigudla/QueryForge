#pragma once

#include "queryforge/query/query_filter.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace queryforge {

struct WeightedQuery {
    std::vector<std::string> where_clauses;
    double weight = 1.0;
};

struct WorkloadSpec {
    std::size_t repeat_count = 1;
    std::vector<WeightedQuery> queries;
};

WorkloadSpec parse_workload_file(const std::string& path);
std::vector<QuerySpec> resolve_workload_queries(const WorkloadSpec& workload,
                                                const TableSchema& schema);

}  // namespace queryforge
