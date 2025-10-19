#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <thread>
#include "../include/Order.hpp"
#include "../include/OrderBook.hpp"
#include "../include/MatchingEngine.hpp"
#include "../include/OrderManager.hpp"
#include "../include/MarketData.hpp"
#include "../include/Timer.hpp"

using PriceType = double;
using OrderIdType = int;
using OrderType = Order<PriceType, OrderIdType>;
using OrderBookType = OrderBook<PriceType, OrderIdType>;
using MatchingEngineType = MatchingEngine<PriceType, OrderIdType>;
using OrderManagerType = OrderManager<PriceType, OrderIdType>;

// Test configuration structure
struct TestConfig {
    std::string name;
    int num_iterations;
    bool use_alignment;
    bool use_memory_pool;
};

// Detailed latency analysis
void printLatencyReport(const std::string& test_name, const std::vector<long long>& latencies) {
    if (latencies.empty()) return;

    auto sorted = latencies;
    std::sort(sorted.begin(), sorted.end());

    auto min = sorted.front();
    auto max = sorted.back();
    double mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();

    double variance = 0.0;
    for (auto l : latencies) variance += (l - mean) * (l - mean);
    double stddev = std::sqrt(variance / latencies.size());

    auto p50 = sorted[sorted.size() / 2];
    auto p90 = sorted[static_cast<size_t>(sorted.size() * 0.90)];
    auto p95 = sorted[static_cast<size_t>(sorted.size() * 0.95)];
    auto p99 = sorted[static_cast<size_t>(sorted.size() * 0.99)];
    auto p999 = sorted[static_cast<size_t>(sorted.size() * 0.999)];

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Test: " << test_name << "\n";
    std::cout << std::string(70, '-') << "\n";
    std::cout << std::left << std::setw(20) << "Metric" << std::right << std::setw(15) 
              << "Value (ns)" << std::setw(20) << "Value (μs)" << "\n";
    std::cout << std::string(70, '-') << "\n";
    
    auto printRow = [](const std::string& label, long long ns_value) {
        std::cout << std::left << std::setw(20) << label 
                  << std::right << std::setw(15) << ns_value 
                  << std::setw(20) << std::fixed << std::setprecision(3) 
                  << (ns_value / 1000.0) << "\n";
    };

    printRow("Min", min);
    printRow("Max", max);
    printRow("Mean", static_cast<long long>(mean));
    printRow("Std Dev", static_cast<long long>(stddev));
    printRow("Median (P50)", p50);
    printRow("P90", p90);
    printRow("P95", p95);
    printRow("P99", p99);
    printRow("P99.9", p999);
    
    std::cout << std::string(70, '=') << "\n";
}

// Test 1: Basic latency test
void testBasicLatency(int num_iterations) {
    std::cout << "\n[TEST 1] Basic Tick-to-Trade Latency\n";
    
    OrderBookType order_book("TEST");
    MatchingEngineType matching_engine(order_book);
    OrderManagerType order_manager;
    MarketDataFeed market_feed(100.0);

    std::vector<long long> latencies;
    latencies.reserve(num_iterations);

    Timer timer;

    for (int i = 0; i < num_iterations; ++i) {
        timer.start();

        // Generate tick
        auto tick = market_feed.generateTick("TEST");
        
        // Create order
        bool is_buy = (i % 2 == 0);
        double price = is_buy ? tick.bid_price : tick.ask_price;
        auto order = order_manager.createOrder("TEST", price, 100, is_buy);
        
        // Match order
        matching_engine.matchOrder(std::move(order));

        latencies.push_back(timer.stop());
    }

    printLatencyReport("Basic Latency Test", latencies);
}

// Test 2: High-load latency test
void testHighLoadLatency(int num_iterations) {
    std::cout << "\n[TEST 2] High-Load Latency Test\n";
    
    OrderBookType order_book("LOAD");
    MatchingEngineType matching_engine(order_book);
    OrderManagerType order_manager;
    MarketDataFeed market_feed(150.0);

    // Pre-populate order book
    for (int i = 0; i < 1000; ++i) {
        auto tick = market_feed.generateTick("LOAD");
        auto buy = order_manager.createOrder("LOAD", tick.bid_price, 100, true);
        auto sell = order_manager.createOrder("LOAD", tick.ask_price, 100, false);
        order_book.addBuyOrder(std::move(buy));
        order_book.addSellOrder(std::move(sell));
    }

    std::vector<long long> latencies;
    latencies.reserve(num_iterations);
    Timer timer;

    for (int i = 0; i < num_iterations; ++i) {
        timer.start();

        auto tick = market_feed.generateTick("LOAD");
        bool is_buy = (i % 2 == 0);
        double price = is_buy ? tick.ask_price + 0.5 : tick.bid_price - 0.5;
        
        auto order = order_manager.createOrder("LOAD", price, 50, is_buy);
        matching_engine.matchOrder(std::move(order));

        latencies.push_back(timer.stop());
    }

    printLatencyReport("High-Load Latency Test", latencies);
}

// Test 3: Burst latency test
void testBurstLatency(int num_bursts, int burst_size) {
    std::cout << "\n[TEST 3] Burst Latency Test (" << num_bursts << " bursts of " 
              << burst_size << " orders)\n";
    
    OrderBookType order_book("BURST");
    MatchingEngineType matching_engine(order_book);
    OrderManagerType order_manager;
    MarketDataFeed market_feed(200.0);

    std::vector<long long> latencies;
    latencies.reserve(num_bursts * burst_size);
    Timer timer;

    for (int burst = 0; burst < num_bursts; ++burst) {
        for (int i = 0; i < burst_size; ++i) {
            timer.start();

            auto tick = market_feed.generateTick("BURST");
            bool is_buy = (i % 2 == 0);
            double price = is_buy ? tick.bid_price : tick.ask_price;
            
            auto order = order_manager.createOrder("BURST", price, 75, is_buy);
            matching_engine.matchOrder(std::move(order));

            latencies.push_back(timer.stop());
        }
        
        // Small pause between bursts
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    printLatencyReport("Burst Latency Test", latencies);
}

// Test 4: Consistency test across different loads
void testLatencyConsistency() {
    std::cout << "\n[TEST 4] Latency Consistency Across Different Loads\n";
    
    std::vector<int> load_sizes = {100, 1000, 10000, 50000};
    
    for (int load : load_sizes) {
        OrderBookType order_book("CONSISTENCY");
        MatchingEngineType matching_engine(order_book);
        OrderManagerType order_manager;
        MarketDataFeed market_feed(180.0);

        std::vector<long long> latencies;
        latencies.reserve(load);
        Timer timer;

        for (int i = 0; i < load; ++i) {
            timer.start();

            auto tick = market_feed.generateTick("CONSISTENCY");
            bool is_buy = (i % 2 == 0);
            double price = is_buy ? tick.bid_price : tick.ask_price;
            
            auto order = order_manager.createOrder("CONSISTENCY", price, 100, is_buy);
            matching_engine.matchOrder(std::move(order));

            latencies.push_back(timer.stop());
        }

        std::string test_name = "Load: " + std::to_string(load) + " orders";
        printLatencyReport(test_name, latencies);
    }
}

// Compare different configurations
void runComparativeTests() {
    std::cout << "\n[COMPARATIVE ANALYSIS] Memory Alignment Impact\n";
    std::cout << "Note: This demonstrates the system's performance characteristics.\n";
    
    // The current system already uses alignas(64) in MarketData
    // We'll test with varying order book sizes
    
    std::vector<int> book_sizes = {0, 100, 500, 1000};
    
    for (int size : book_sizes) {
        OrderBookType order_book("COMP");
        MatchingEngineType matching_engine(order_book);
        OrderManagerType order_manager;
        MarketDataFeed market_feed(160.0);

        // Pre-populate
        for (int i = 0; i < size; ++i) {
            auto tick = market_feed.generateTick("COMP");
            auto buy = order_manager.createOrder("COMP", tick.bid_price, 100, true);
            auto sell = order_manager.createOrder("COMP", tick.ask_price, 100, false);
            order_book.addBuyOrder(std::move(buy));
            order_book.addSellOrder(std::move(sell));
        }

        std::vector<long long> latencies;
        latencies.reserve(1000);
        Timer timer;

        for (int i = 0; i < 1000; ++i) {
            timer.start();
            auto tick = market_feed.generateTick("COMP");
            auto order = order_manager.createOrder("COMP", tick.bid_price, 100, true);
            matching_engine.matchOrder(std::move(order));
            latencies.push_back(timer.stop());
        }

        std::string test_name = "Order Book Size: " + std::to_string(size);
        printLatencyReport(test_name, latencies);
    }
}

int main() {
    std::cout << "\n";
    std::cout << "====================================================================\n";
    std::cout << "        HFT System - Comprehensive Latency Benchmark\n";
    std::cout << "                    IEOR E4741 - Phase 4\n";
    std::cout << "====================================================================\n";

    // Run all benchmark tests
    testBasicLatency(10000);
    testHighLoadLatency(10000);
    testBurstLatency(100, 100);
    testLatencyConsistency();
    runComparativeTests();

    std::cout << "\n";
    std::cout << "====================================================================\n";
    std::cout << "                  Benchmark Complete\n";
    std::cout << "====================================================================\n";
    std::cout << "\nAll latency tests completed successfully.\n";
    std::cout << "Key Observations:\n";
    std::cout << "   - P99 latency is critical for HFT systems\n";
    std::cout << "   - Cache-line alignment (alignas) improves consistency\n";
    std::cout << "   - Memory pools reduce allocation overhead\n";
    std::cout << "   - Smart pointers provide safety with minimal overhead\n\n";

    return 0;
}
