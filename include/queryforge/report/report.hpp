#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct BenchmarkMetadata {
    int warmup = 0;
    int runs = 0;
    std::uint32_t seed = 0;
    std::size_t repeat_count = 1;
    std::string os;
    std::string compiler;
    std::string build_type;
};

BenchmarkMetadata collect_benchmark_metadata(int warmup,
                                             int runs,
                                             std::uint32_t seed,
                                             std::size_t repeat_count);
