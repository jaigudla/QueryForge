#include "queryforge/util/format.hpp"

#include <cmath>
#include <format>
#include <string>

std::string format_with_commas(std::uint64_t value) {
    const std::string digits = std::to_string(value);
    const int first_group = static_cast<int>(digits.size() % 3);
    std::string result;

    if (first_group > 0) {
        result.append(digits, 0, static_cast<std::size_t>(first_group));
    }

    for (int i = first_group; i < static_cast<int>(digits.size()); i += 3) {
        if (!result.empty()) {
            result.push_back(',');
        }
        result.append(digits, static_cast<std::size_t>(i), 3);
    }

    return result;
}

std::string format_ms(double ms) {
    const double rounded = std::round(ms * 10.0) / 10.0;
    return std::format("{:.1f} ms", rounded);
}
