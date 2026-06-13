#include "queryforge/data/data_loader.hpp"

#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string trim(std::string_view value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return std::string{value.substr(start, end - start + 1)};
}

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }
    return fields;
}

template <typename T>
T parse_number(const std::string& value, const std::string& field, std::size_t line_number) {
    T parsed{};
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw std::runtime_error("Invalid " + field + " value on CSV line " +
                                 std::to_string(line_number) + ": " + value);
    }
    return parsed;
}

double parse_double(const std::string& value, const std::string& field, std::size_t line_number) {
    try {
        std::size_t parsed_chars = 0;
        const double parsed = std::stod(value, &parsed_chars);
        if (parsed_chars != value.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid " + field + " value on CSV line " +
                                 std::to_string(line_number) + ": " + value);
    }
}

}  // namespace

std::vector<TradeEvent> load_trade_events_csv(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open CSV input: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("CSV input is empty: " + path);
    }

    const std::vector<std::string> header = split_csv_line(line);
    const std::vector<std::string> expected = {"symbol", "timestamp", "price", "quantity"};
    if (header != expected) {
        throw std::runtime_error(
            "CSV header must be: symbol,timestamp,price,quantity");
    }

    std::vector<TradeEvent> events;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        ++line_number;
        if (trim(line).empty()) {
            continue;
        }

        const std::vector<std::string> fields = split_csv_line(line);
        if (fields.size() != expected.size()) {
            throw std::runtime_error("CSV line " + std::to_string(line_number) +
                                     " must contain 4 fields");
        }
        if (fields[0].empty()) {
            throw std::runtime_error("Missing symbol on CSV line " + std::to_string(line_number));
        }

        events.push_back(TradeEvent{
            fields[0],
            parse_number<int64_t>(fields[1], "timestamp", line_number),
            parse_double(fields[2], "price", line_number),
            parse_number<int>(fields[3], "quantity", line_number),
        });
    }

    if (events.empty()) {
        throw std::runtime_error("CSV input has no trade rows: " + path);
    }

    return events;
}
