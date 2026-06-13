#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

void execute_run(std::size_t rows,
                 const std::string& symbol,
                 int warmup,
                 int runs,
                 std::uint32_t seed);
