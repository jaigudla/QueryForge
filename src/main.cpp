#include <CLI/CLI.hpp>

#include "queryforge/cli/run_command.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    CLI::App app{
        "QueryForge — benchmark in-memory query strategies for C++ developers"
    };

    RunOptions run_options;

    CLI::App* run = app.add_subcommand("run", "Benchmark query strategies over trade events");
    run->add_option("--rows", run_options.rows)->check(CLI::PositiveNumber);
    run->add_option("--input", run_options.input_path, "CSV file with symbol,timestamp,price,quantity");
    run->add_option("--symbol", run_options.symbol, "Shortcut for --where symbol=<value>");
    run->add_option("--where", run_options.where_clauses, "Query filter, e.g. symbol=AAPL or price<250");
    run->add_option("--strategy", run_options.strategies, "Strategy: all, linear_scan, hash_index, sorted_index");
    run->add_option("--schema", run_options.schema, "CSV schema, e.g. user_id:int64,country:string");
    run->add_option("--delimiter", run_options.delimiter, "Single-character CSV delimiter");
    run->add_flag("--infer-schema,!--no-infer-schema",
                  run_options.infer_schema,
                  "Infer CSV column types when --schema is not provided");
    run->add_option("--warmup", run_options.warmup)
        ->check(CLI::Range(0, std::numeric_limits<int>::max()));
    run->add_option("--runs", run_options.runs)->check(CLI::Range(1, std::numeric_limits<int>::max()));
    run->add_option("--seed", run_options.seed);
    run->add_option("--repeat-count", run_options.repeat_count)->check(CLI::PositiveNumber);
    run->add_option("--output", run_options.output)->check(CLI::IsMember({"text", "json"}));

    CLI11_PARSE(app, argc, argv);

    if (*run) {
        execute_run(run_options);
    }

    return 0;
}
