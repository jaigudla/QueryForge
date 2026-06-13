#include "queryforge/cli/run_command.hpp"

#include "queryforge/data/dataset_generator.hpp"
#include "queryforge/query/linear_scan.hpp"
#include "queryforge/util/format.hpp"

#include <format>
#include <iostream>

void execute_run(std::size_t rows,
                 const std::string& symbol,
                 int warmup,
                 int runs,
                 std::uint32_t seed) {
    const std::vector<TradeEvent> events = generate_dataset(rows, seed);
    const ScanResult result = linear_scan_benchmark(events, symbol, runs, warmup);

    std::cout << "QueryForge\n\n";
    std::cout << "Dataset:\n";
    std::cout << std::format("  Rows: {}\n", format_with_commas(static_cast<std::uint64_t>(rows)));
    std::cout << std::format("  Symbols: {}\n\n", kSymbolPoolSize);
    std::cout << "Query:\n";
    std::cout << std::format("  symbol == \"{}\"\n\n", symbol);
    std::cout << "Result:\n";
    print_benchmark_table("linear_scan", result.stats, result.matches);
}
