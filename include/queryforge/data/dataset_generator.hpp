#pragma once

#include "queryforge/core/trade_event.hpp"

#include <cstdint>
#include <string>
#include <vector>

constexpr std::size_t kSymbolPoolSize = 500;

std::vector<std::string> build_symbol_pool();
std::vector<TradeEvent> generate_dataset(std::size_t rows, std::uint32_t seed = 42);
