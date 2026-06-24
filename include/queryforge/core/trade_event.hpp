#pragma once

#include <cstdint>
#include <string>

namespace queryforge {

struct TradeEvent {
    std::string symbol;
    int64_t timestamp;
    double price;
    int quantity;
};

}  // namespace queryforge
