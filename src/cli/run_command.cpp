#include "queryforge/cli/run_command.hpp"

#include "queryforge/data/dataset_generator.hpp"
#include "queryforge/query/linear_scan.hpp"
#include "queryforge/util/format.hpp"

#include <format>
#include <iostream>

void execute_run(std::size_t rows, const std::string& symbol) {
    const std::vector<TradeEvent> events = generate_dataset(rows);
    const ScanResult result = linear_scan_benchmark(events, symbol);

    std::cout << "QueryForge\n\n";
    std::cout << "Dataset:\n";
    std::cout << std::format("  Rows: {}\n", format_with_commas(static_cast<std::uint64_t>(rows)));
    std::cout << std::format("  Symbols: {}\n\n", kSymbolPoolSize);
    std::cout << "Query:\n";
    std::cout << std::format("  symbol == \"{}\"\n\n", symbol);
    std::cout << "Result:\n";
    std::cout << "  Strategy: linear_scan\n";
    std::cout << std::format("  Matches: {}\n", format_with_commas(result.matches));
    std::cout << std::format("  p50: {}\n", format_ms(result.p50_ms));
}
