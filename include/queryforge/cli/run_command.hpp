#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct RunOptions {
    std::size_t rows = 1'000'000;
    std::string input_path;
    std::string symbol = "AAPL";
    std::vector<std::string> where_clauses;
    std::vector<std::string> strategies;
    int warmup = 5;
    int runs = 30;
    std::uint32_t seed = 42;
    std::size_t repeat_count = 1;
    std::string output = "text";
};

void execute_run(const RunOptions& options);
