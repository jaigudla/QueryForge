#include <CLI/CLI.hpp>

#include "queryforge/cli/run_command.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{
        "QueryForge — benchmark in-memory query strategies for C++ developers"
    };

    std::size_t rows = 1'000'000;
    std::string symbol = "AAPL";
    int warmup = 5;
    int runs = 30;
    std::uint32_t seed = 42;

    CLI::App* run = app.add_subcommand("run", "Generate dataset and benchmark linear scan");
    run->add_option("--rows", rows)->required()->check(CLI::PositiveNumber);
    run->add_option("--symbol", symbol);
    run->add_option("--warmup", warmup)->check(CLI::Range(0, std::numeric_limits<int>::max()));
    run->add_option("--runs", runs)->check(CLI::Range(1, std::numeric_limits<int>::max()));
    run->add_option("--seed", seed);

    CLI11_PARSE(app, argc, argv);

    if (*run) {
        execute_run(rows, symbol, warmup, runs, seed);
    }

    return 0;
}
