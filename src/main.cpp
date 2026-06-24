#include <CLI/CLI.hpp>

#include "queryforge/cli/compare_command.hpp"
#include "queryforge/cli/inspect_command.hpp"
#include "queryforge/cli/run_command.hpp"
#include "queryforge/version.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    CLI::App app{"QueryForge — benchmark in-memory query strategies for C++ developers"};

    app.set_version_flag("--version", QUERYFORGE_VERSION);
    app.set_help_all_flag("--help-all", "Show all options");

    queryforge::RunOptions run_options;
    queryforge::InspectOptions inspect_options;
    queryforge::CompareOptions compare_options;

    CLI::App* run =
        app.add_subcommand("run", "Benchmark query strategies over in-memory tables");
    run->add_option("--rows", run_options.rows)->check(CLI::PositiveNumber);
    run->add_option("--input", run_options.input_path, "CSV file path");
    run->add_option("--symbol", run_options.symbol,
                    "Deprecated shortcut for --where symbol=<value>");
    run->add_option("--where", run_options.where_clauses,
                    "Query filter, e.g. country=US or price<250");
    run->add_option("--strategy", run_options.strategies,
                    "Strategy: all, linear_scan, hash_index, composite_hash, sorted_index, columnar_scan");
    run->add_option("--schema", run_options.schema, "CSV schema, e.g. user_id:int64,country:string");
    run->add_option("--schema-file", run_options.schema_file, "Path to JSON or line-based schema file");
    run->add_option("--workload", run_options.workload_path, "Weighted multi-query workload file");
    run->add_option("--delimiter", run_options.delimiter, "Single-character CSV delimiter");
    run->add_flag("--infer-schema,!--no-infer-schema", run_options.infer_schema,
                  "Infer CSV column types when --schema is not provided");
    run->add_option("--warmup", run_options.warmup)
        ->check(CLI::Range(0, std::numeric_limits<int>::max()));
    run->add_option("--runs", run_options.runs)->check(CLI::Range(1, std::numeric_limits<int>::max()));
    run->add_option("--seed", run_options.seed);
    run->add_option("--repeat-count", run_options.repeat_count)->check(CLI::PositiveNumber);
    run->add_option("--output", run_options.output)->check(CLI::IsMember({"text", "json"}));
    run->add_flag("--memory-profile", run_options.memory_profile, "Report peak resident memory");

    CLI::App* inspect = app.add_subcommand("inspect", "Preview CSV schema and sample rows");
    inspect->add_option("file", inspect_options.input_path, "CSV file path")->required();
    inspect->add_option("--schema", inspect_options.schema);
    inspect->add_option("--schema-file", inspect_options.schema_file);
    inspect->add_option("--delimiter", inspect_options.delimiter);
    inspect->add_flag("--infer-schema,!--no-infer-schema", inspect_options.infer_schema);
    inspect->add_option("--rows", inspect_options.preview_rows)->check(CLI::PositiveNumber);
    inspect->add_option("--output", inspect_options.output)->check(CLI::IsMember({"text", "json"}));

    CLI::App* compare =
        app.add_subcommand("compare", "Compare two JSON benchmark result files");
    compare->add_option("baseline", compare_options.baseline_path, "Baseline JSON file")->required();
    compare->add_option("candidate", compare_options.candidate_path, "Candidate JSON file")
        ->required();
    compare->add_option("--output", compare_options.output)->check(CLI::IsMember({"text", "json"}));

    CLI11_PARSE(app, argc, argv);

    if (*run) {
        const queryforge::Result<void> result = queryforge::execute_run(run_options);
        if (!result) {
            std::cerr << "Error: " << result.error().message << '\n';
            return 1;
        }
    } else if (*inspect) {
        const queryforge::Result<void> result = queryforge::execute_inspect(inspect_options);
        if (!result) {
            std::cerr << "Error: " << result.error().message << '\n';
            return 1;
        }
    } else if (*compare) {
        const queryforge::Result<void> result = queryforge::execute_compare(compare_options);
        if (!result) {
            std::cerr << "Error: " << result.error().message << '\n';
            return 1;
        }
    }

    return 0;
}
