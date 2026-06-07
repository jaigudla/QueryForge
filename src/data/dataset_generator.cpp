#include "queryforge/data/dataset_generator.hpp"

#include <array>
#include <format>
#include <random>

namespace {

constexpr std::array<const char*, 30> kSeedTickers = {
    "AAPL", "MSFT", "NVDA", "GOOGL", "AMZN", "META", "TSLA", "BRK.B", "JPM", "V",
    "UNH",  "XOM",  "LLY",  "JNJ",   "WMT",  "MA",   "PG",   "AVGO",  "HD",  "CVX",
    "MRK",  "ABBV", "KO",   "PEP",   "COST", "TMO",  "ADBE", "MCD",   "CSCO", "ACN",
};

}  // namespace

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
