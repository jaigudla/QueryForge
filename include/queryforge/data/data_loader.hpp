#pragma once

#include "queryforge/core/trade_event.hpp"
#include "queryforge/data/table.hpp"

#include <optional>
#include <string>
#include <vector>

struct CsvLoadOptions {
    std::optional<TableSchema> schema;
    char delimiter = ',';
    bool infer_schema = true;
};

Table load_csv_table(const std::string& path, const CsvLoadOptions& options = {});
std::vector<TradeEvent> load_trade_events_csv(const std::string& path);
