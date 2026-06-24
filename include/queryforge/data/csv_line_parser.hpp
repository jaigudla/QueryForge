#pragma once

#include <string>
#include <vector>

namespace queryforge {

std::vector<std::string> parse_csv_line(const std::string& line, char delimiter);

}  // namespace queryforge
