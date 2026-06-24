#include "queryforge/data/csv_line_parser.hpp"

#include <stdexcept>
#include <string>

namespace queryforge {

std::vector<std::string> parse_csv_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (in_quotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                current.push_back(ch);
            }
        } else if (ch == '"') {
            in_quotes = true;
        } else if (ch == delimiter) {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    if (in_quotes) {
        throw std::runtime_error("Unclosed quoted CSV field");
    }

    fields.push_back(current);
    return fields;
}

}  // namespace queryforge
