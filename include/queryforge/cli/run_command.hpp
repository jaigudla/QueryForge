#pragma once

#include "queryforge/core/result.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace queryforge {

struct RunOptions {
    std::size_t rows = 1'000'000;
    std::string input_path;
    std::string symbol = "AAPL";
    std::vector<std::string> where_clauses;
    std::vector<std::string> strategies;
    std::string schema;
    std::string schema_file;
    std::string workload_path;
    std::string delimiter = ",";
    bool infer_schema = true;
    int warmup = 5;
    int runs = 30;
    std::uint32_t seed = 42;
    std::size_t repeat_count = 1;
    std::string output = "text";
    bool memory_profile = false;
};

Result<void> execute_run(const RunOptions& options);

}  // namespace queryforge
