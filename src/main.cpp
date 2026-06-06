#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
    CLI::App app{
        "QueryForge — benchmark in-memory query strategies for C++ developers"
    };
    CLI11_PARSE(app, argc, argv);
    return 0;
}
