#include "../include/MarketData.hpp"
#include <random>

MarketDataFeed::MarketDataFeed(double base_price)
    : rng(std::random_device{}()),
      price_dist(-5.0, 5.0),  // Price variation range
      size_dist(50, 500)      // Size variation range
{
}

MarketData MarketDataFeed::generateTick(const std::string& symbol) {
    static double base_price = 150.0;
    
    // Generate random price movements
    double price_change = price_dist(rng);
    base_price += price_change * 0.1; // Small incremental changes
    
    // Ensure positive prices
    if (base_price < 1.0) base_price = 100.0;
    
    // Generate bid-ask spread (typically 0.01 to 0.05)
    double spread = 0.01 + (price_dist(rng) + 5.0) / 1000.0;
    
    double bid = base_price - spread / 2.0;
    double ask = base_price + spread / 2.0;
    
    int bid_size = size_dist(rng);
    int ask_size = size_dist(rng);
    
    return MarketData(symbol, bid, ask, bid_size, ask_size);
}

std::vector<MarketData> MarketDataFeed::generateTicks(const std::string& symbol, int count) {
    std::vector<MarketData> ticks;
    ticks.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        ticks.push_back(generateTick(symbol));
    }
    
    return ticks;
}
