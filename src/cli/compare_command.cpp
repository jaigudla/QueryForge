#include "queryforge/cli/compare_command.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace queryforge {
namespace {

std::string read_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open file: " + path);
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

double extract_number_after(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\": ";
    const std::size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        return 0.0;
    }
    return std::stod(json.substr(pos + needle.size()));
}

std::size_t extract_strategy_matches(const std::string& json, const std::string& strategy) {
    const std::string needle = "\"name\": \"" + strategy + "\"";
    const std::size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        return 0;
    }
    const std::size_t matches_pos = json.find("\"matches\": ", pos);
    if (matches_pos == std::string::npos) {
        return 0;
    }
    return static_cast<std::size_t>(
        std::stoull(json.substr(matches_pos + std::string("\"matches\": ").size())));
}

}  // namespace

Result<void> execute_compare(const CompareOptions& options) {
    try {
        const std::string baseline = read_file(options.baseline_path);
        const std::string candidate = read_file(options.candidate_path);

        const std::size_t baseline_rows =
            static_cast<std::size_t>(extract_number_after(baseline, "rows"));
        const std::size_t candidate_rows =
            static_cast<std::size_t>(extract_number_after(candidate, "rows"));

        const std::size_t baseline_matches =
            extract_strategy_matches(baseline, "linear_scan");
        const std::size_t candidate_matches =
            extract_strategy_matches(candidate, "linear_scan");

        if (options.output == "json") {
            std::cout << "{\n";
            std::cout << "  \"rows_delta\": " << static_cast<long long>(candidate_rows) -
                             static_cast<long long>(baseline_rows) << ",\n";
            std::cout << "  \"linear_scan_matches_delta\": "
                      << static_cast<long long>(candidate_matches) -
                             static_cast<long long>(baseline_matches)
                      << "\n}\n";
            return Result<void>::ok();
        }

        std::cout << "QueryForge compare\n\n";
        std::cout << "Baseline: " << options.baseline_path << '\n';
        std::cout << "Candidate: " << options.candidate_path << "\n\n";
        std::cout << "Rows delta: "
                  << static_cast<long long>(candidate_rows) -
                         static_cast<long long>(baseline_rows)
                  << '\n';
        std::cout << "linear_scan matches delta: "
                  << static_cast<long long>(candidate_matches) -
                         static_cast<long long>(baseline_matches)
                  << '\n';

        return Result<void>::ok();
    } catch (const std::exception& ex) {
        return Result<void>::fail(ex.what());
    }
}

}  // namespace queryforge
