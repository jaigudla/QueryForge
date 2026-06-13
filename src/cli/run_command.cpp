#include "queryforge/cli/run_command.hpp"

#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/data_loader.hpp"
#include "queryforge/data/dataset_generator.hpp"
#include "queryforge/data/table.hpp"
#include "queryforge/query/query_filter.hpp"
#include "queryforge/query/strategy.hpp"
#include "queryforge/recommend/recommendation.hpp"
#include "queryforge/report/report.hpp"
#include "queryforge/util/format.hpp"

#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

std::string json_escape(const std::string& value) {
    std::ostringstream escaped;
    for (const char ch : value) {
        switch (ch) {
            case '"':
                escaped << "\\\"";
                break;
            case '\\':
                escaped << "\\\\";
                break;
            case '\n':
                escaped << "\\n";
                break;
            default:
                escaped << ch;
                break;
        }
    }
    return escaped.str();
}

std::vector<std::string> effective_where_clauses(const RunOptions& options) {
    if (!options.where_clauses.empty()) {
        return options.where_clauses;
    }
    return {"symbol=" + options.symbol};
}

char parse_delimiter(const std::string& delimiter) {
    if (delimiter.size() != 1) {
        throw std::runtime_error("--delimiter must be exactly one character");
    }
    return delimiter[0];
}

void print_text_report(const RunOptions& options,
                       const BenchmarkMetadata& metadata,
                       const QuerySpec& query,
                       const std::vector<StrategyBenchmarkResult>& results,
                       const Recommendation& recommendation,
                       std::size_t row_count) {
    const bool using_input = !options.input_path.empty();

    std::cout << "QueryForge\n\n";
    std::cout << "Dataset:\n";
    std::cout << std::format("  Rows: {}\n", format_with_commas(static_cast<std::uint64_t>(row_count)));
    if (using_input) {
        std::cout << std::format("  Input: {}\n\n", options.input_path);
    } else {
        std::cout << std::format("  Symbols: {}\n\n", kSymbolPoolSize);
    }

    std::cout << "Query:\n";
    std::cout << std::format("  {}\n\n", describe_query(query));

    std::cout << "Benchmark:\n";
    std::cout << std::format("  Warmup: {}\n", metadata.warmup);
    std::cout << std::format("  Runs: {}\n", metadata.runs);
    std::cout << std::format("  Seed: {}\n", metadata.seed);
    std::cout << std::format("  Repeat count: {}\n", metadata.repeat_count);
    std::cout << std::format("  OS: {}\n", metadata.os);
    std::cout << std::format("  Compiler: {}\n", metadata.compiler);
    std::cout << std::format("  Build: {}\n\n", metadata.build_type);

    std::cout << "Result:\n";
    print_strategy_table(results);

    std::cout << "\nRecommendation:\n";
    std::cout << std::format("  {}\n", recommendation.message);
}

void print_json_report(const RunOptions& options,
                       const BenchmarkMetadata& metadata,
                       const QuerySpec& query,
                       const std::vector<StrategyBenchmarkResult>& results,
                       const Recommendation& recommendation,
                       std::size_t row_count) {
    std::cout << "{\n";
    std::cout << std::format("  \"rows\": {},\n", row_count);
    std::cout << std::format("  \"input\": \"{}\",\n", json_escape(options.input_path));
    std::cout << std::format("  \"query\": \"{}\",\n", json_escape(describe_query(query)));
    std::cout << "  \"benchmark\": {\n";
    std::cout << std::format("    \"warmup\": {},\n", metadata.warmup);
    std::cout << std::format("    \"runs\": {},\n", metadata.runs);
    std::cout << std::format("    \"seed\": {},\n", metadata.seed);
    std::cout << std::format("    \"repeat_count\": {},\n", metadata.repeat_count);
    std::cout << std::format("    \"os\": \"{}\",\n", json_escape(metadata.os));
    std::cout << std::format("    \"compiler\": \"{}\",\n", json_escape(metadata.compiler));
    std::cout << std::format("    \"build_type\": \"{}\"\n", json_escape(metadata.build_type));
    std::cout << "  },\n";
    std::cout << "  \"strategies\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const StrategyBenchmarkResult& result = results[i];
        std::cout << "    {\n";
        std::cout << std::format("      \"name\": \"{}\",\n", json_escape(result.strategy));
        std::cout << std::format("      \"applicable\": {},\n", result.applicable ? "true" : "false");
        std::cout << std::format("      \"note\": \"{}\",\n", json_escape(result.note));
        std::cout << std::format("      \"build_ms\": {},\n", result.build_time_ms);
        std::cout << std::format("      \"p50_ms\": {},\n", result.stats.p50_ms);
        std::cout << std::format("      \"p95_ms\": {},\n", result.stats.p95_ms);
        std::cout << std::format("      \"p99_ms\": {},\n", result.stats.p99_ms);
        std::cout << std::format("      \"avg_ms\": {},\n", result.stats.avg_ms);
        std::cout << std::format("      \"min_ms\": {},\n", result.stats.min_ms);
        std::cout << std::format("      \"max_ms\": {},\n", result.stats.max_ms);
        std::cout << std::format("      \"memory_bytes\": {},\n", result.memory_bytes);
        std::cout << std::format("      \"matches\": {}\n", result.matches);
        std::cout << "    }" << (i + 1 == results.size() ? "\n" : ",\n");
    }
    std::cout << "  ],\n";
    std::cout << "  \"recommendation\": {\n";
    std::cout << std::format("    \"strategy\": \"{}\",\n", json_escape(recommendation.strategy));
    std::cout << std::format("    \"estimated_total_ms\": {},\n", recommendation.estimated_total_ms);
    std::cout << std::format("    \"message\": \"{}\"\n", json_escape(recommendation.message));
    std::cout << "  }\n";
    std::cout << "}\n";
}

}  // namespace

void execute_run(const RunOptions& options) {
    Table table;
    if (options.input_path.empty()) {
        table = trade_events_to_table(generate_dataset(options.rows, options.seed));
    } else {
        CsvLoadOptions load_options;
        load_options.delimiter = parse_delimiter(options.delimiter);
        load_options.infer_schema = options.infer_schema;
        if (!options.schema.empty()) {
            load_options.schema = parse_table_schema(options.schema);
        }
        table = load_csv_table(options.input_path, load_options);
    }

    const QuerySpec query = parse_query_filters(effective_where_clauses(options), table.schema);
    const std::vector<StrategyBenchmarkResult> results =
        benchmark_strategies(table, query, options.strategies, options.runs, options.warmup);
    const BenchmarkMetadata metadata =
        collect_benchmark_metadata(options.warmup, options.runs, options.seed, options.repeat_count);
    const Recommendation recommendation = make_recommendation(results, options.repeat_count);

    if (options.output == "json") {
        print_json_report(options, metadata, query, results, recommendation, table.rows.size());
    } else {
        print_text_report(options, metadata, query, results, recommendation, table.rows.size());
    }
}
