#include "queryforge/report/report.hpp"

BenchmarkMetadata collect_benchmark_metadata(int warmup,
                                             int runs,
                                             std::uint32_t seed,
                                             std::size_t repeat_count) {
    BenchmarkMetadata metadata;
    metadata.warmup = warmup;
    metadata.runs = runs;
    metadata.seed = seed;
    metadata.repeat_count = repeat_count;

#if defined(_WIN32)
    metadata.os = "Windows";
#elif defined(__APPLE__)
    metadata.os = "macOS";
#elif defined(__linux__)
    metadata.os = "Linux";
#else
    metadata.os = "Unknown";
#endif

#if defined(_MSC_VER)
    metadata.compiler = "MSVC " + std::to_string(_MSC_VER);
#elif defined(__clang__)
    metadata.compiler = "Clang " + std::to_string(__clang_major__) + "." +
                        std::to_string(__clang_minor__);
#elif defined(__GNUC__)
    metadata.compiler = "GCC " + std::to_string(__GNUC__) + "." +
                        std::to_string(__GNUC_MINOR__);
#else
    metadata.compiler = "Unknown";
#endif

#if defined(NDEBUG)
    metadata.build_type = "Release";
#else
    metadata.build_type = "Debug";
#endif

    return metadata;
}
