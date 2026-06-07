#include <CLI/CLI.hpp>

#include "queryforge/cli/run_command.hpp"

#include <cstddef>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{
        "QueryForge — benchmark in-memory query strategies for C++ developers"
    };

    std::size_t rows = 1'000'000;
    std::string symbol = "AAPL";

    CLI::App* run = app.add_subcommand("run", "Generate dataset and benchmark linear scan");
    run->add_option("--rows", rows)->required();
    run->add_option("--symbol", symbol);

    CLI11_PARSE(app, argc, argv);

    if (*run) {
        execute_run(rows, symbol);
    }

    return 0;
}
