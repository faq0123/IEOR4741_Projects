#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <fstream>
#include <cstdint>
#include <cmath>

struct alignas(64) MarketData {
    int instrument_id;
    double price;
    std::chrono::high_resolution_clock::time_point timestamp;
};

class MarketDataFeed {
public:
    MarketDataFeed(std::vector<MarketData>& ref) : data(ref) {}

    void generateData(int num_ticks) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> price_dist(100.0, 200.0);

        data.reserve(num_ticks);
        for (int i = 0; i < num_ticks; ++i) {
            MarketData md;
            md.instrument_id = i % 10;
            md.price = price_dist(gen);
            md.timestamp = std::chrono::high_resolution_clock::now();
            data.push_back(md);
        }
    }

private:
    std::vector<MarketData>& data;
};

struct alignas(64) Order {
    int instrument_id;
    double price;
    bool is_buy;
    std::chrono::high_resolution_clock::time_point timestamp;
    uint32_t signal_mask;  // bit0=signal1, bit1=signal2, bit2=signal3
};

class TradeEngine {
public:
    TradeEngine(const std::vector<MarketData>& feed)
        : market_data(feed) {
        signal_counts[0] = signal_counts[1] = signal_counts[2] = 0;
    }

    void process() {
        orders.reserve(market_data.size() / 3);
        latencies.reserve(market_data.size());

        for (const auto& tick : market_data) {
            updateHistory(tick);

            // New Signal1: volatility breakout, returns -1(sell) / +1(buy) / 0(none)
            int s1_dir = signal1_breakout(tick, 2.0);
            bool s2 = signal2_mean_deviation(tick);
            bool s3 = signal3_momentum(tick);

            bool buy = false, sell = false;
            uint32_t mask = 0;

            if (s1_dir != 0) {
                if (s1_dir > 0) buy = true; else sell = true;
                mask |= 1u;
            }
            if (s2) {
                double avg = getAvg(tick.instrument_id);
                if (tick.price < avg) buy = true; else sell = true;
                mask |= 2u;
            }
            if (s3) {
                buy = true;
                mask |= 4u;
            }

            if (mask != 0u) {
                auto now = std::chrono::high_resolution_clock::now();
                double px = tick.price + (buy ? 0.01 : (sell ? -0.01 : 0.0));
                Order o { tick.instrument_id, px, buy, now, mask };
                orders.push_back(o);

                long long latency = std::chrono::duration_cast<std::chrono::nanoseconds>(now - tick.timestamp).count();
                latencies.push_back(latency);

                if (mask & 1u) ++signal_counts[0];
                if (mask & 2u) ++signal_counts[1];
                if (mask & 4u) ++signal_counts[2];
            }
        }
    }

    void reportStats() {
        long long sum = 0, max_latency = 0;
        for (auto l : latencies) {
            sum += l;
            if (l > max_latency) max_latency = l;
        }
        long long avg = latencies.empty() ? 0 : sum / (long long)latencies.size();

        std::cout << "\n--- Performance Report ---\n";
        std::cout << "Total Market Ticks Processed: " << market_data.size() << "\n";
        std::cout << "Total Orders Placed: " << orders.size() << "\n";
        std::cout << "Average Tick-to-Trade Latency (ns): " << avg << "\n";
        std::cout << "Maximum Tick-to-Trade Latency (ns): " << max_latency << "\n";
        std::cout << "Signal Contributions (count; overlapping counted per signal):\n";
        std::cout << "  Signal1 (Breakout): " << signal_counts[0] << "\n";
        std::cout << "  Signal2 (Mean Deviation):      " << signal_counts[1] << "\n";
        std::cout << "  Signal3 (Momentum):            " << signal_counts[2] << "\n";
    }

    void exportCSV(const std::string& path = "orders.csv") {
        std::ofstream ofs(path);
        ofs << "instrument_id,price,is_buy,timestamp_ns,signal_mask\n";
        for (const auto& o : orders) {
            long long tns = std::chrono::duration_cast<std::chrono::nanoseconds>(o.timestamp.time_since_epoch()).count();
            ofs << o.instrument_id << "," << o.price << "," << (o.is_buy ? 1 : 0) << "," << tns << "," << o.signal_mask << "\n";
        }
        ofs.close();
        std::cout << "Orders exported to " << path << "\n";
    }

private:
    const std::vector<MarketData>& market_data;
    std::vector<Order> orders;
    std::vector<long long> latencies;
    std::unordered_map<int, std::vector<double>> price_history;
    long long signal_counts[3];

    void updateHistory(const MarketData& tick) {
        auto& hist = price_history[tick.instrument_id];
        hist.push_back(tick.price);
        if (hist.size() > 10) hist.erase(hist.begin());
    }

    double getAvg(int id) {
        auto& hist = price_history[id];
        if (hist.empty()) return 0.0;
        double sum = 0.0;
        for (double p : hist) sum += p;
        return sum / hist.size();
    }

    // Signal1: mean +(-) k * std
    int signal1_breakout(const MarketData& tick, double k_sigma) {
        auto& hist = price_history[tick.instrument_id];
        if (hist.size() < 10) return 0; // 数据不足不触发

        double avg = getAvg(tick.instrument_id);
        double sumsq = 0.0;
        for (double p : hist) {
            double d = p - avg;
            sumsq += d * d;
        }
        double stddev = std::sqrt(sumsq / hist.size());
        double upper = avg + k_sigma * stddev;
        double lower = avg - k_sigma * stddev;

        if (tick.price > upper) return +1; 
        if (tick.price < lower) return -1; 
        return 0;
    }

    // Signal2
    bool signal2_mean_deviation(const MarketData& tick) {
        auto it = price_history.find(tick.instrument_id);
        if (it == price_history.end() || it->second.size() < 5) return false;
        double avg = getAvg(tick.instrument_id);
        return tick.price < avg * 0.9 || tick.price > avg * 1.1;
    }

    // Signal3
    bool signal3_momentum(const MarketData& tick) {
        auto& hist = price_history[tick.instrument_id];
        if (hist.size() < 3) return false;
        double diff1 = hist[hist.size() - 2] - hist[hist.size() - 3];
        double diff2 = hist[hist.size() - 1] - hist[hist.size() - 2];
        return diff1 > 0 && diff2 > 0;
    }
};

int main() {
    std::vector<MarketData> feed;
    MarketDataFeed generator(feed);

    auto start = std::chrono::high_resolution_clock::now();
    generator.generateData(100000);

    TradeEngine engine(feed);
    engine.process();

    auto end = std::chrono::high_resolution_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    engine.reportStats();
    std::cout << "Total Runtime (ms): " << runtime << std::endl;

    // engine.exportCSV("orders_log.csv");
    engine.exportCSV("/Users/kinsley/Desktop/课/programming/orders_log.csv");
    return 0;
}
