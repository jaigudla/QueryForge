#pragma once

#include <cstdint>
#include <string>

struct TradeEvent {
    std::string symbol;
    int64_t timestamp;
    double price;
    int quantity;
};
