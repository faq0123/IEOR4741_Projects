#pragma once
#include "MatchingEngine.hpp"
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <iomanip>
#include <sstream>

// RAII-based Trade Logger
template <typename PriceType, typename OrderIdType>
class TradeLogger {
public:
    using TradeType = Trade<PriceType, OrderIdType>;

private:
    std::vector<TradeType> trades;
    std::unique_ptr<std::ofstream> log_file;
    std::string log_filename;
    bool batch_mode;
    size_t batch_size;

public:
    explicit TradeLogger(const std::string& filename = "trades.log", 
                        bool batch = true, size_t batch_sz = 1000)
        : log_filename(filename), batch_mode(batch), batch_size(batch_sz) {
        // Pre-allocate vector for performance
        trades.reserve(10000);
        
        // Open log file using RAII
        log_file = std::make_unique<std::ofstream>(filename);
        if (log_file->is_open()) {
            writeHeader();
        }
    }

    // Destructor ensures file is flushed and closed (RAII)
    ~TradeLogger() {
        flush();
        if (log_file && log_file->is_open()) {
            log_file->close();
        }
    }

    // Disable copy (unique ownership of file)
    TradeLogger(const TradeLogger&) = delete;
    TradeLogger& operator=(const TradeLogger&) = delete;

    // Allow move
    TradeLogger(TradeLogger&&) noexcept = default;
    TradeLogger& operator=(TradeLogger&&) noexcept = default;

    // Log a single trade
    void logTrade(const TradeType& trade) {
        trades.push_back(trade);
        
        if (!batch_mode || trades.size() >= batch_size) {
            flush();
        }
    }

    // Log multiple trades
    void logTrades(const std::vector<TradeType>& new_trades) {
        trades.insert(trades.end(), new_trades.begin(), new_trades.end());
        
        if (!batch_mode || trades.size() >= batch_size) {
            flush();
        }
    }

    // Flush pending trades to file
    void flush() {
        if (!log_file || !log_file->is_open() || trades.empty()) {
            return;
        }

        for (const auto& trade : trades) {
            writeTrade(trade);
        }

        log_file->flush();
        trades.clear();
    }

    // Get statistics
    size_t getPendingCount() const { return trades.size(); }

    const std::string& getFilename() const { return log_filename; }

private:
    void writeHeader() {
        if (!log_file || !log_file->is_open()) return;
        
        *log_file << "Timestamp,BuyOrderID,SellOrderID,Symbol,Price,Quantity\n";
    }

    void writeTrade(const TradeType& trade) {
        if (!log_file || !log_file->is_open()) return;

        auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            trade.timestamp.time_since_epoch()
        ).count();

        *log_file << time_ns << ","
                 << trade.buy_order_id << ","
                 << trade.sell_order_id << ","
                 << trade.symbol << ","
                 << std::fixed << std::setprecision(2) << trade.price << ","
                 << trade.quantity << "\n";
    }

public:
    // Generate a summary report
    std::string generateSummary(const std::vector<TradeType>& all_trades) const {
        if (all_trades.empty()) {
            return "No trades to summarize.\n";
        }

        std::ostringstream oss;
        oss << "\n=== Trade Summary ===\n";
        oss << "Total Trades: " << all_trades.size() << "\n";

        // Calculate total volume
        long long total_volume = 0;
        double total_value = 0.0;
        for (const auto& trade : all_trades) {
            total_volume += trade.quantity;
            total_value += trade.price * trade.quantity;
        }

        oss << "Total Volume: " << total_volume << " shares\n";
        oss << "Total Value: $" << std::fixed << std::setprecision(2) << total_value << "\n";
        
        if (total_volume > 0) {
            oss << "Average Price: $" << (total_value / total_volume) << "\n";
        }

        oss << "===================\n";
        return oss.str();
    }
};
