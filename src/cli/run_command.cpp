#include "queryforge/cli/run_command.hpp"

#include "queryforge/data/csv_schema.hpp"
#include "queryforge/data/data_loader.hpp"
#include "queryforge/data/dataset_generator.hpp"
#include "queryforge/data/table.hpp"
#include "queryforge/data/workload.hpp"
#include "queryforge/query/query_filter.hpp"
#include "queryforge/query/strategy.hpp"
#include "queryforge/recommend/recommendation.hpp"
#include "queryforge/report/report.hpp"
#include "queryforge/util/format.hpp"
#include "queryforge/util/memory.hpp"

#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace queryforge {
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

TableSchema resolve_schema(const RunOptions& options) {
    if (!options.schema_file.empty()) {
        return parse_schema_file(options.schema_file);
    }
    if (!options.schema.empty()) {
        return parse_table_schema(options.schema);
    }
    return default_trade_schema();
}

Table load_dataset(const RunOptions& options) {
    if (!options.input_path.empty()) {
        CsvLoadOptions load_options;
        load_options.delimiter = parse_delimiter(options.delimiter);
        load_options.infer_schema = options.infer_schema;
        if (!options.schema_file.empty()) {
            load_options.schema = parse_schema_file(options.schema_file);
        } else if (!options.schema.empty()) {
            load_options.schema = parse_table_schema(options.schema);
        }
        return load_csv_table(options.input_path, load_options);
    }

    const TableSchema schema = resolve_schema(options);
    if (schema.columns.size() == 4 && schema.columns[0].name == "symbol" &&
        schema.columns[1].name == "timestamp" && schema.columns[2].name == "price" &&
        schema.columns[3].name == "quantity" && options.schema.empty() &&
        options.schema_file.empty()) {
        return trade_events_to_table(generate_dataset(options.rows, options.seed));
    }
    return generate_table(schema, options.rows, options.seed);
}

void print_text_report(const RunOptions& options,
                       const BenchmarkMetadata& metadata,
                       const std::string& query_description,
                       const std::vector<StrategyBenchmarkResult>& results,
                       const Recommendation& recommendation,
                       std::size_t row_count,
                       std::size_t peak_rss) {
    const bool using_input = !options.input_path.empty();

    std::cout << "QueryForge\n\n";
    std::cout << "Dataset:\n";
    std::cout << std::format("  Rows: {}\n", format_with_commas(static_cast<std::uint64_t>(row_count)));
    if (using_input) {
        std::cout << std::format("  Input: {}\n\n", options.input_path);
    } else {
        std::cout << std::format("  Seed: {}\n\n", metadata.seed);
    }

    std::cout << "Query:\n";
    std::cout << std::format("  {}\n\n", query_description);

    std::cout << "Benchmark:\n";
    std::cout << std::format("  Warmup: {}\n", metadata.warmup);
    std::cout << std::format("  Runs: {}\n", metadata.runs);
    std::cout << std::format("  Repeat count: {}\n", metadata.repeat_count);
    std::cout << std::format("  OS: {}\n", metadata.os);
    std::cout << std::format("  Compiler: {}\n", metadata.compiler);
    std::cout << std::format("  Build: {}\n", metadata.build_type);
    std::cout << std::format("  Clock: {}\n", metadata.clock_source);
    if (options.memory_profile) {
        std::cout << std::format("  Peak RSS: {}\n", format_bytes(peak_rss));
    }
    std::cout << '\n';

    std::cout << "Result:\n";
    print_strategy_table(results);

    std::cout << "\nRecommendation:\n";
    std::cout << std::format("  {}\n", recommendation.message);
}

void print_json_report(const RunOptions& options,
                       const BenchmarkMetadata& metadata,
                       const std::string& query_description,
                       const std::vector<StrategyBenchmarkResult>& results,
                       const Recommendation& recommendation,
                       std::size_t row_count,
                       std::size_t peak_rss) {
    std::cout << "{\n";
    std::cout << std::format("  \"rows\": {},\n", row_count);
    std::cout << std::format("  \"input\": \"{}\",\n", json_escape(options.input_path));
    std::cout << std::format("  \"query\": \"{}\",\n", json_escape(query_description));
    std::cout << "  \"benchmark\": {\n";
    std::cout << std::format("    \"warmup\": {},\n", metadata.warmup);
    std::cout << std::format("    \"runs\": {},\n", metadata.runs);
    std::cout << std::format("    \"seed\": {},\n", metadata.seed);
    std::cout << std::format("    \"repeat_count\": {},\n", metadata.repeat_count);
    std::cout << std::format("    \"os\": \"{}\",\n", json_escape(metadata.os));
    std::cout << std::format("    \"compiler\": \"{}\",\n", json_escape(metadata.compiler));
    std::cout << std::format("    \"build_type\": \"{}\",\n", json_escape(metadata.build_type));
    std::cout << std::format("    \"clock_source\": \"{}\",\n", json_escape(metadata.clock_source));
    if (options.memory_profile) {
        std::cout << std::format("    \"peak_rss_bytes\": {}\n", peak_rss);
    } else {
        std::cout << "    \"peak_rss_bytes\": null\n";
    }
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
        std::cout << std::format("      \"stddev_ms\": {},\n", result.stats.stddev_ms);
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

Recommendation run_single_query(const Table& table,
                                const RunOptions& options,
                                const QuerySpec& query,
                                std::vector<StrategyBenchmarkResult>& results_out) {
    results_out =
        benchmark_strategies(table, query, options.strategies, options.runs, options.warmup);
    return make_recommendation(results_out, options.repeat_count);
}

Recommendation run_workload(const Table& table,
                            const RunOptions& options,
                            const WorkloadSpec& workload,
                            std::vector<StrategyBenchmarkResult>& results_out) {
    const std::vector<std::string> strategies = normalize_strategy_names(options.strategies);
    std::vector<WeightedStrategyResult> weighted_results;
    weighted_results.reserve(strategies.size());

    for (const std::string& strategy : strategies) {
        WeightedStrategyResult aggregate{strategy, 0.0, 0.0, 0, true};
        double max_build = 0.0;

        for (const WeightedQuery& weighted_query : workload.queries) {
            const QuerySpec query =
                parse_query_filters(weighted_query.where_clauses, table.schema);
            const auto results =
                benchmark_strategies(table, query, {strategy}, options.runs, options.warmup);
            if (results.empty() || !results[0].applicable) {
                aggregate.applicable = false;
                break;
            }
            aggregate.weighted_avg_ms += weighted_query.weight * results[0].stats.avg_ms;
            max_build = std::max(max_build, results[0].build_time_ms);
            aggregate.memory_bytes = std::max(aggregate.memory_bytes, results[0].memory_bytes);
        }

        aggregate.build_time_ms = max_build;
        weighted_results.push_back(aggregate);
    }

    if (!workload.queries.empty()) {
        results_out = benchmark_strategies(
            table,
            parse_query_filters(workload.queries.front().where_clauses, table.schema),
            options.strategies,
            options.runs,
            options.warmup);
    }

    return make_weighted_recommendation(weighted_results, workload.repeat_count);
}

}  // namespace

Result<void> execute_run(const RunOptions& options) {
    try {
        const Table table = load_dataset(options);
        const std::size_t peak_rss = options.memory_profile ? get_peak_rss_bytes() : 0;

        std::vector<StrategyBenchmarkResult> results;
        Recommendation recommendation;
        std::string query_description;
        std::size_t repeat_count = options.repeat_count;

        if (!options.workload_path.empty()) {
            const WorkloadSpec workload = parse_workload_file(options.workload_path);
            repeat_count = workload.repeat_count;
            recommendation = run_workload(table, options, workload, results);
            query_description = "weighted workload (" + options.workload_path + ")";
        } else {
            const QuerySpec query =
                parse_query_filters(effective_where_clauses(options), table.schema);
            query_description = describe_query(query);
            recommendation = run_single_query(table, options, query, results);
        }

        BenchmarkMetadata metadata =
            collect_benchmark_metadata(options.warmup, options.runs, options.seed, repeat_count);

        if (options.output == "json") {
            print_json_report(options, metadata, query_description, results, recommendation,
                              table.rows.size(), peak_rss);
        } else {
            print_text_report(options, metadata, query_description, results, recommendation,
                              table.rows.size(), peak_rss);
        }

        return Result<void>::ok();
    } catch (const std::exception& ex) {
        return Result<void>::fail(ex.what());
    }
}

}  // namespace queryforge
