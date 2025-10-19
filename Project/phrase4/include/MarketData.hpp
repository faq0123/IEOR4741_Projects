#pragma once
#include <string>
#include <chrono>
#include <vector>
#include <random>

// Cache-line aligned market data structure for optimal performance
struct alignas(64) MarketData {
    std::string symbol;
    double bid_price;
    double ask_price;
    int bid_size;
    int ask_size;
    std::chrono::high_resolution_clock::time_point timestamp;

    MarketData(std::string sym, double bid, double ask, int bid_sz = 100, int ask_sz = 100)
        : symbol(std::move(sym)), bid_price(bid), ask_price(ask), 
          bid_size(bid_sz), ask_size(ask_sz),
          timestamp(std::chrono::high_resolution_clock::now()) {}
    
    MarketData() : symbol(""), bid_price(0.0), ask_price(0.0), 
                   bid_size(0), ask_size(0),
                   timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Market Data Feed Simulator
class MarketDataFeed {
private:
    std::mt19937 rng;
    std::uniform_real_distribution<double> price_dist;
    std::uniform_int_distribution<int> size_dist;
    
public:
    MarketDataFeed(double base_price = 150.0);
    
    // Generate simulated market data tick
    MarketData generateTick(const std::string& symbol);
    
    // Generate multiple ticks
    std::vector<MarketData> generateTicks(const std::string& symbol, int count);
};
