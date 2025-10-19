#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include "../include/Order.hpp"
#include "../include/OrderBook.hpp"
#include "../include/MatchingEngine.hpp"
#include "../include/OrderManager.hpp"
#include "../include/TradeLogger.hpp"
#include "../include/MarketData.hpp"
#include "../include/Timer.hpp"

using PriceType = double;
using OrderIdType = int;
using OrderType = Order<PriceType, OrderIdType>;
using OrderBookType = OrderBook<PriceType, OrderIdType>;
using MatchingEngineType = MatchingEngine<PriceType, OrderIdType>;
using OrderManagerType = OrderManager<PriceType, OrderIdType>;
using TradeLoggerType = TradeLogger<PriceType, OrderIdType>;
using TradeType = Trade<PriceType, OrderIdType>;

// Analyze latency statistics
void analyzeLatencies(const std::vector<long long>& latencies) {
    if (latencies.empty()) {
        std::cout << "No latency data to analyze.\n";
        return;
    }

    // Sort for percentile calculations
    auto sorted_latencies = latencies;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());

    auto min = sorted_latencies.front();
    auto max = sorted_latencies.back();
    double mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();

    // Calculate standard deviation
    double variance = 0.0;
    for (auto l : latencies) {
        variance += (l - mean) * (l - mean);
    }
    double stddev = std::sqrt(variance / latencies.size());

    // Calculate percentiles
    auto p50 = sorted_latencies[static_cast<size_t>(latencies.size() * 0.50)];
    auto p95 = sorted_latencies[static_cast<size_t>(latencies.size() * 0.95)];
    auto p99 = sorted_latencies[static_cast<size_t>(latencies.size() * 0.99)];

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Tick-to-Trade Latency Analysis (nanoseconds)\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Sample Size:     " << latencies.size() << "\n";
    std::cout << "Min:             " << min << " ns\n";
    std::cout << "Max:             " << max << " ns\n";
    std::cout << "Mean:            " << std::fixed << std::setprecision(2) << mean << " ns\n";
    std::cout << "Std Dev:         " << stddev << " ns\n";
    std::cout << "Median (P50):    " << p50 << " ns\n";
    std::cout << "P95:             " << p95 << " ns\n";
    std::cout << "P99:             " << p99 << " ns\n";
    std::cout << std::string(60, '=') << "\n\n";
}

// Run a basic HFT simulation
void runBasicSimulation(int num_ticks) {
    std::cout << "\n*** Running Basic HFT Simulation ***\n";
    std::cout << "Number of ticks: " << num_ticks << "\n\n";

    // Initialize components
    OrderBookType order_book("AAPL");
    MatchingEngineType matching_engine(order_book);
    OrderManagerType order_manager;
    TradeLoggerType trade_logger("trades_basic.log");
    MarketDataFeed market_feed(150.0);

    std::vector<long long> latencies;
    latencies.reserve(num_ticks);

    Timer timer;

    // Simulate market data and order flow
    for (int i = 0; i < num_ticks; ++i) {
        timer.start();

        // Generate market data tick
        auto market_data = market_feed.generateTick("AAPL");

        // Create orders based on market data
        bool is_buy = (i % 2 == 0);
        double price = is_buy ? market_data.bid_price : market_data.ask_price;
        int quantity = 100 + (i % 5) * 20;

        // Create and submit order
        auto order = order_manager.createOrder("AAPL", price, quantity, is_buy);
        
        // Match order
        auto trades = matching_engine.matchOrder(std::move(order));

        // Log trades
        if (!trades.empty()) {
            trade_logger.logTrades(trades);
        }

        // Record latency
        long long latency = timer.stop();
        latencies.push_back(latency);
    }

    // Flush remaining trades
    trade_logger.flush();

    // Analyze results
    analyzeLatencies(latencies);

    // Print trade summary
    std::cout << trade_logger.generateSummary(matching_engine.getTrades());
    std::cout << "Total trades executed: " << matching_engine.getTradeCount() << "\n";
    std::cout << "Orders in book - Buy: " << order_book.getBuyOrderCount() 
              << ", Sell: " << order_book.getSellOrderCount() << "\n\n";
}

// Run an aggressive matching simulation
void runAggressiveSimulation(int num_orders) {
    std::cout << "\n*** Running Aggressive Matching Simulation ***\n";
    std::cout << "Number of orders: " << num_orders << "\n\n";

    OrderBookType order_book("MSFT");
    MatchingEngineType matching_engine(order_book);
    OrderManagerType order_manager;
    TradeLoggerType trade_logger("trades_aggressive.log");
    MarketDataFeed market_feed(300.0);

    std::vector<long long> latencies;
    latencies.reserve(num_orders);

    Timer timer;

    // First, populate the order book with resting orders
    std::cout << "Populating order book...\n";
    for (int i = 0; i < num_orders / 2; ++i) {
        auto market_data = market_feed.generateTick("MSFT");
        
        // Add buy order
        auto buy_order = order_manager.createOrder("MSFT", market_data.bid_price, 100, true);
        order_book.addBuyOrder(std::move(buy_order));
        
        // Add sell order
        auto sell_order = order_manager.createOrder("MSFT", market_data.ask_price, 100, false);
        order_book.addSellOrder(std::move(sell_order));
    }

    std::cout << "Order book populated. Starting matching...\n";

    // Now send aggressive orders that cross the spread
    for (int i = 0; i < num_orders / 2; ++i) {
        timer.start();

        auto market_data = market_feed.generateTick("MSFT");
        
        // Create aggressive order that will match
        bool is_buy = (i % 2 == 0);
        // Buy at ask price or sell at bid price (aggressive)
        double price = is_buy ? market_data.ask_price + 1.0 : market_data.bid_price - 1.0;
        
        auto order = order_manager.createOrder("MSFT", price, 100, is_buy);
        auto trades = matching_engine.matchOrder(std::move(order));

        if (!trades.empty()) {
            trade_logger.logTrades(trades);
        }

        long long latency = timer.stop();
        latencies.push_back(latency);
    }

    trade_logger.flush();

    // Analyze results
    analyzeLatencies(latencies);
    
    std::cout << trade_logger.generateSummary(matching_engine.getTrades());
    std::cout << "Total trades executed: " << matching_engine.getTradeCount() << "\n";
    std::cout << "Orders remaining in book - Buy: " << order_book.getBuyOrderCount() 
              << ", Sell: " << order_book.getSellOrderCount() << "\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "====================================================================\n";
    std::cout << "    High-Frequency Trading System - Phase 4 Project\n";
    std::cout << "                    IEOR E4741\n";
    std::cout << "====================================================================\n";

    // Run different simulation scenarios
    
    // Scenario 1: Basic simulation with 10K ticks
    runBasicSimulation(10000);

    // Scenario 2: Aggressive matching with 5K orders
    runAggressiveSimulation(5000);

    // Scenario 3: Stress test with 100K ticks
    std::cout << "\n*** Running Stress Test (100K ticks) ***\n";
    std::cout << "This may take a moment...\n\n";
    
    OrderBookType stress_book("GOOGL");
    MatchingEngineType stress_engine(stress_book);
    OrderManagerType stress_manager;
    TradeLoggerType stress_logger("trades_stress.log");
    MarketDataFeed stress_feed(2800.0);

    std::vector<long long> stress_latencies;
    stress_latencies.reserve(100000);
    
    Timer stress_timer;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        stress_timer.start();
        
        auto market_data = stress_feed.generateTick("GOOGL");
        bool is_buy = (i % 3 != 0);  // 2/3 buy, 1/3 sell
        double price = is_buy ? market_data.bid_price : market_data.ask_price;
        
        auto order = stress_manager.createOrder("GOOGL", price, 50 + (i % 10) * 10, is_buy);
        auto trades = stress_engine.matchOrder(std::move(order));
        
        if (!trades.empty()) {
            stress_logger.logTrades(trades);
        }
        
        stress_latencies.push_back(stress_timer.stop());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    stress_logger.flush();

    analyzeLatencies(stress_latencies);
    
    std::cout << "Total execution time: " << total_time << " ms\n";
    std::cout << "Throughput: " << (100000.0 / total_time * 1000.0) << " ticks/second\n";
    std::cout << stress_logger.generateSummary(stress_engine.getTrades());
    std::cout << "Total trades: " << stress_engine.getTradeCount() << "\n\n";

    std::cout << "\nAll simulations completed successfully.\n";
    std::cout << "Check trades_*.log files for detailed trade logs.\n\n";

    return 0;
}
