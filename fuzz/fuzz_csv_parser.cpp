#include "queryforge/data/csv_line_parser.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) {
        return 0;
    }

    const std::string input(reinterpret_cast<const char*>(data), size);
    try {
        (void)queryforge::parse_csv_line(input, ',');
        (void)queryforge::parse_csv_line(input, '|');
        (void)queryforge::parse_csv_line(input, '\t');
    } catch (const std::exception&) {
    }
    return 0;
}
