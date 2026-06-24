#include "queryforge/data/dataset_generator.hpp"

#include <array>
#include <format>
#include <random>
#include <stdexcept>

namespace queryforge {
namespace {

constexpr std::array<const char*, 30> kSeedTickers = {
    "AAPL", "MSFT", "NVDA", "GOOGL", "AMZN", "META", "TSLA", "BRK.B", "JPM", "V",
    "UNH",  "XOM",  "LLY",  "JNJ",   "WMT",  "MA",   "PG",   "AVGO",  "HD",  "CVX",
    "MRK",  "ABBV", "KO",   "PEP",   "COST", "TMO",  "ADBE", "MCD",   "CSCO", "ACN",
};

constexpr std::array<const char*, 8> kCountries = {
    "US", "CA", "GB", "DE", "FR", "JP", "AU", "BR",
};

CellValue generate_cell(ColumnType type, std::mt19937& rng, std::size_t row_index) {
    switch (type) {
        case ColumnType::String: {
            std::uniform_int_distribution<std::size_t> pick(0, kCountries.size() - 1);
            if (row_index % 17 == 0) {
                return std::string{kSeedTickers[row_index % kSeedTickers.size()]};
            }
            return std::string{kCountries[pick(rng)]};
        }
        case ColumnType::Int64: {
            std::uniform_int_distribution<int64_t> dist(1, 10'000);
            return dist(rng);
        }
        case ColumnType::Double: {
            std::uniform_real_distribution<double> dist(0.0, 1000.0);
            return dist(rng);
        }
        case ColumnType::Bool: {
            std::uniform_int_distribution<int> dist(0, 1);
            return dist(rng) == 1;
        }
    }
    throw std::runtime_error("Unsupported column type");
}

}  // namespace

TableSchema default_trade_schema() {
    return TableSchema{{
        {"symbol", ColumnType::String},
        {"timestamp", ColumnType::Int64},
        {"price", ColumnType::Double},
        {"quantity", ColumnType::Int64},
    }};
}

std::vector<std::string> build_symbol_pool() {
    std::vector<std::string> pool;
    pool.reserve(kSymbolPoolSize);

    for (const char* ticker : kSeedTickers) {
        pool.emplace_back(ticker);
    }

    for (std::size_t i = pool.size(); i < kSymbolPoolSize; ++i) {
        pool.push_back(std::format("TCK{:03d}", i - kSeedTickers.size() + 1));
    }

    return pool;
}

std::vector<TradeEvent> generate_dataset(std::size_t rows, std::uint32_t seed) {
    const std::vector<std::string> symbols = build_symbol_pool();
    std::vector<TradeEvent> events;
    events.reserve(rows);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<std::size_t> symbol_dist(0, symbols.size() - 1);
    std::uniform_int_distribution<int64_t> timestamp_dist(1'600'000'000'000'000'000LL,
                                                          1'700'000'000'000'000'000LL);
    std::uniform_real_distribution<double> price_dist(10.0, 500.0);
    std::uniform_int_distribution<int> quantity_dist(1, 10'000);

    for (std::size_t i = 0; i < rows; ++i) {
        events.push_back(TradeEvent{
            symbols[symbol_dist(rng)],
            timestamp_dist(rng),
            price_dist(rng),
            quantity_dist(rng),
        });
    }

    return events;
}

Table generate_table(const TableSchema& schema, std::size_t rows, std::uint32_t seed) {
    if (schema.columns.empty()) {
        throw std::runtime_error("Synthetic data generation requires a schema");
    }

    Table table;
    table.schema = schema;
    table.rows.reserve(rows);

    std::mt19937 rng(seed);
    for (std::size_t row_index = 0; row_index < rows; ++row_index) {
        std::vector<CellValue> row;
        row.reserve(schema.columns.size());
        for (const ColumnSchema& column : schema.columns) {
            row.push_back(generate_cell(column.type, rng, row_index));
        }
        table.rows.push_back(std::move(row));
    }

    return table;
}

}  // namespace queryforge
