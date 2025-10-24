#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <fstream>
#include <atomic>
#include <vector>
#include <iomanip>

using namespace std;
using namespace std::chrono;

//  Configuration 
struct Config {
    int marketDataRate = 1000;      // updates per second
    double strategyThreshold = 2.0;  // price delta threshold
    int simulationSeconds = 10;      // duration
    bool verboseLogging = false;     // detailed logs
    int numStrategyThreads = 1;      // parallel strategies
};

//  Data Structures 
struct PriceUpdate {
    double price;
    steady_clock::time_point timestamp;
};

struct Order {
    string side;
    double price;
    steady_clock::time_point createdAt;
    steady_clock::time_point routedAt;
};

//  Thread-Safe Queue 
template<typename T>
class ThreadSafeQueue {
private:
    queue<T> q;
    mutex mtx;
    condition_variable cv;
public:
    void push(const T& item) {
        lock_guard<mutex> lock(mtx);
        q.push(item);
        cv.notify_one();
    }
    
    bool pop(T& item, const atomic<bool>& running) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, &running] { return !q.empty() || !running.load(); });
        if (!running.load() && q.empty()) return false;
        item = q.front();
        q.pop();
        return true;
    }
    
    size_t size() {
        lock_guard<mutex> lock(mtx);
        return q.size();
    }
    
    void notifyAll() {
        cv.notify_all();
    }
};

//  Performance Monitor 
class PerformanceMonitor {
private:
    mutex mtx;
    vector<double> latencies;
    atomic<int> orderCount{0};
    steady_clock::time_point startTime;
    
public:
    void start() {
        startTime = steady_clock::now();
    }
    
    void recordOrder(double latencyUs) {
        lock_guard<mutex> lock(mtx);
        latencies.push_back(latencyUs);
        orderCount++;
    }
    
    void exportMetrics(const string& filename, const Config& config) {
        lock_guard<mutex> lock(mtx);
        auto duration = duration_cast<milliseconds>(steady_clock::now() - startTime).count() / 1000.0;
        
        ofstream file(filename);
        file << "Metric,Value\n";
        file << "Duration(s)," << duration << "\n";
        file << "TotalOrders," << orderCount << "\n";
        file << "Throughput(orders/s)," << orderCount / duration << "\n";
        
        if (!latencies.empty()) {
            double sum = 0, minLat = latencies[0], maxLat = latencies[0];
            for (auto lat : latencies) {
                sum += lat;
                minLat = min(minLat, lat);
                maxLat = max(maxLat, lat);
            }
            file << "AvgLatency(us)," << sum / latencies.size() << "\n";
            file << "MinLatency(us)," << minLat << "\n";
            file << "MaxLatency(us)," << maxLat << "\n";
        }
        
        file << "MarketDataRate," << config.marketDataRate << "\n";
        file << "StrategyThreads," << config.numStrategyThreads << "\n";
        file.close();
    }
    
    void printSummary() {
        lock_guard<mutex> lock(mtx);
        cout << "\n Performance Summary \n";
        cout << "Total Orders: " << orderCount << "\n";
        if (!latencies.empty()) {
            double sum = 0;
            for (auto lat : latencies) sum += lat;
            cout << "Avg Latency: " << fixed << setprecision(2) 
                 << sum / latencies.size() << " us\n";
        }
        cout << "=\n";
    }
};

//  Market Data Feed 
class MarketDataFeed {
private:
    ThreadSafeQueue<PriceUpdate>& priceQueue;
    atomic<bool>& running;
    int ratePerSecond;
    double basePrice = 100.0;
    
public:
    MarketDataFeed(ThreadSafeQueue<PriceUpdate>& pq, atomic<bool>& run, int rate)
        : priceQueue(pq), running(run), ratePerSecond(rate) {}
    
    void operator()() {
        auto sleepTime = microseconds(1000000 / ratePerSecond);
        
        while (running) {
            // Simulate price movement
            double priceChange = (rand() % 100 - 50) / 10.0;
            basePrice += priceChange;
            basePrice = max(50.0, min(150.0, basePrice)); // bounds
            
            PriceUpdate update{basePrice, steady_clock::now()};
            priceQueue.push(update);
            
            this_thread::sleep_for(sleepTime);
        }
    }
};

//  Strategy Engine 
class StrategyEngine {
private:
    ThreadSafeQueue<PriceUpdate>& priceQueue;
    ThreadSafeQueue<Order>& orderQueue;
    atomic<bool>& running;
    double threshold;
    double lastPrice = 0;
    int id;
    
public:
    StrategyEngine(ThreadSafeQueue<PriceUpdate>& pq, ThreadSafeQueue<Order>& oq,
                   atomic<bool>& run, double thresh, int engineId)
        : priceQueue(pq), orderQueue(oq), running(run), threshold(thresh), id(engineId) {}
    
    void operator()() {
        while (running) {
            PriceUpdate update;
            if (!priceQueue.pop(update, running)) break;
            
            if (lastPrice == 0) {
                lastPrice = update.price;
                continue;
            }
            
            double delta = update.price - lastPrice;
            
            // Strategy: trade on significant price movements
            if (abs(delta) > threshold) {
                Order order{
                    delta < 0 ? "BUY" : "SELL",
                    update.price,
                    update.timestamp,
                    steady_clock::now()
                };
                orderQueue.push(order);
            }
            
            lastPrice = update.price;
        }
    }
};

//  Order Router 
class OrderRouter {
private:
    ThreadSafeQueue<Order>& orderQueue;
    atomic<bool>& running;
    PerformanceMonitor& monitor;
    ofstream& logFile;
    bool verbose;
    
public:
    OrderRouter(ThreadSafeQueue<Order>& oq, atomic<bool>& run, 
                PerformanceMonitor& pm, ofstream& log, bool verb)
        : orderQueue(oq), running(run), monitor(pm), logFile(log), verbose(verb) {}
    
    void operator()() {
        while (running) {
            Order order;
            if (!orderQueue.pop(order, running)) break;
            
            auto now = steady_clock::now();
            auto latency = duration_cast<microseconds>(now - order.createdAt).count();
            
            // Log order
            logFile << order.side << "," 
                    << fixed << setprecision(2) << order.price << "," 
                    << latency << "\n";
            
            if (verbose) {
                cout << "[ORDER] " << order.side << " @ " << order.price 
                     << " | Latency: " << latency << " us\n";
            }
            
            monitor.recordOrder(latency);
        }
    }
};

//  Configuration Loader 
Config loadConfig(const string& filename) {
    Config config;
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Config file not found. Using defaults.\n";
        return config;
    }
    
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == string::npos) continue;
        
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        
        if (key == "market_data_rate") config.marketDataRate = stoi(value);
        else if (key == "strategy_threshold") config.strategyThreshold = stod(value);
        else if (key == "simulation_seconds") config.simulationSeconds = stoi(value);
        else if (key == "verbose_logging") config.verboseLogging = (value == "true");
        else if (key == "num_strategy_threads") config.numStrategyThreads = stoi(value);
    }
    
    file.close();
    return config;
}

//  Main 
int main(int argc, char* argv[]) {
    // Load configuration
    string configFile = (argc > 1) ? argv[1] : "config.txt";
    Config config = loadConfig(configFile);
    
    cout << " HFT System Starting \n";
    cout << "Market Data Rate: " << config.marketDataRate << " updates/s\n";
    cout << "Strategy Threshold: " << config.strategyThreshold << "\n";
    cout << "Strategy Threads: " << config.numStrategyThreads << "\n";
    cout << "Simulation Duration: " << config.simulationSeconds << "s\n";
    cout << "=\n\n";
    
    // Initialize components
    atomic<bool> running{true};
    ThreadSafeQueue<PriceUpdate> priceQueue;
    ThreadSafeQueue<Order> orderQueue;
    PerformanceMonitor monitor;
    
    ofstream logFile("orders.csv");
    logFile << "Side,Price,Latency_us\n";
    
    // Start performance monitoring
    monitor.start();
    
    // Launch threads
    vector<thread> threads;
    
    // Market data feed
    MarketDataFeed feedFunc(priceQueue, running, config.marketDataRate);
    threads.emplace_back(feedFunc);
    
    // Strategy engines
    for (int i = 0; i < config.numStrategyThreads; i++) {
        StrategyEngine strategyFunc(priceQueue, orderQueue, running, 
                                    config.strategyThreshold, i);
        threads.emplace_back(strategyFunc);
    }
    
    // Order router
    OrderRouter routerFunc(orderQueue, running, monitor, logFile, config.verboseLogging);
    threads.emplace_back(routerFunc);
    
    // Run simulation
    this_thread::sleep_for(seconds(config.simulationSeconds));
    
    // Shutdown
    cout << "\nShutting down...\n";
    running = false;
    priceQueue.notifyAll();
    orderQueue.notifyAll();
    
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    logFile.close();
    
    // Export results
    monitor.exportMetrics("performance.csv", config);
    monitor.printSummary();
    
    cout << "\nResults saved:\n";
    cout << "  - orders.csv (order log)\n";
    cout << "  - performance.csv (metrics)\n";
    
    return 0;
}