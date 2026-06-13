#pragma once

#include "queryforge/core/trade_event.hpp"

#include <string>
#include <vector>

std::vector<TradeEvent> load_trade_events_csv(const std::string& path);
