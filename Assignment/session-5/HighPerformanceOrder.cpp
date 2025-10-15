#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include <cassert>

using namespace std;
using namespace std::chrono;

// Phase 1

enum class Side : uint8_t { BUY = 0, SELL = 1 };

struct alignas(64) Order {
    uint64_t id;
    uint32_t price;
    uint32_t quantity;
    Side side;
    
    Order() : id(0), price(0), quantity(0), side(Side::BUY) {}
    Order(uint64_t i, uint32_t p, uint32_t q, Side s) 
        : id(i), price(p), quantity(q), side(s) {}
    
    bool isBuy() const { return side == Side::BUY; }
};

struct alignas(64) PriceLevel {
    uint32_t price;
    uint64_t totalQty;
    uint32_t orderCount;
    
    PriceLevel() : price(0), totalQty(0), orderCount(0) {}
    
    void addOrder(uint32_t qty) {
        totalQty += qty;
        orderCount++;
    }
    
    void removeOrder(uint32_t qty) {
        totalQty -= qty;
        orderCount--;
    }
    
    void amendOrder(uint32_t oldQty, uint32_t newQty) {
        totalQty = totalQty - oldQty + newQty;
    }
    
    bool isEmpty() const { return orderCount == 0; }
};

struct TopOfBook {
    uint32_t bidPrice;
    uint64_t bidQty;
    uint32_t askPrice;
    uint64_t askQty;
    
    TopOfBook() : bidPrice(0), bidQty(0), askPrice(UINT32_MAX), askQty(0) {}
};


// Phase 2

class OrderBookBaseline {
private:
    struct OrderInfo {
        uint32_t price;
        uint32_t quantity;
        Side side;
        size_t levelIndex;
    };
    
    unordered_map<uint64_t, OrderInfo> orders_;
    vector<PriceLevel> bidLevels_;
    vector<PriceLevel> askLevels_;
    
    size_t findLevel(vector<PriceLevel>& levels, uint32_t price) {
        for (size_t i = 0; i < levels.size(); ++i) {
            if (levels[i].price == price) return i;
        }
        return levels.size();
    }
    
    size_t getOrCreateLevel(vector<PriceLevel>& levels, uint32_t price) {
        size_t idx = findLevel(levels, price);
        if (idx == levels.size()) {
            levels.emplace_back();
            levels.back().price = price;
        }
        return idx;
    }

public:
    OrderBookBaseline() {
        orders_.reserve(100000);
        bidLevels_.reserve(1000);
        askLevels_.reserve(1000);
    }
    
    void newOrder(const Order& order) {
        auto& levels = order.isBuy() ? bidLevels_ : askLevels_;
        size_t levelIdx = getOrCreateLevel(levels, order.price);
        levels[levelIdx].addOrder(order.quantity);
        orders_[order.id] = {order.price, order.quantity, order.side, levelIdx};
    }
    
    void amendOrder(uint64_t orderId, uint32_t newQty) {
        auto it = orders_.find(orderId);
        if (it == orders_.end()) return;
        
        auto& info = it->second;
        auto& levels = info.side == Side::BUY ? bidLevels_ : askLevels_;
        levels[info.levelIndex].amendOrder(info.quantity, newQty);
        info.quantity = newQty;
    }
    
    void deleteOrder(uint64_t orderId) {
        auto it = orders_.find(orderId);
        if (it == orders_.end()) return;
        
        auto& info = it->second;
        auto& levels = info.side == Side::BUY ? bidLevels_ : askLevels_;
        levels[info.levelIndex].removeOrder(info.quantity);
        orders_.erase(it);
    }
    
    TopOfBook topOfBook() const {
        TopOfBook tob;
        uint32_t maxBid = 0;
        for (const auto& level : bidLevels_) {
            if (!level.isEmpty() && level.price > maxBid) {
                maxBid = level.price;
                tob.bidQty = level.totalQty;
            }
        }
        tob.bidPrice = maxBid;
        
        uint32_t minAsk = UINT32_MAX;
        for (const auto& level : askLevels_) {
            if (!level.isEmpty() && level.price < minAsk) {
                minAsk = level.price;
                tob.askQty = level.totalQty;
            }
        }
        tob.askPrice = minAsk;
        return tob;
    }
    
    size_t orderCount(uint32_t price, Side side) const {
        const auto& levels = side == Side::BUY ? bidLevels_ : askLevels_;
        for (const auto& level : levels) {
            if (level.price == price) return level.orderCount;
        }
        return 0;
    }
    
    uint64_t totalVolume(uint32_t price, Side side) const {
        const auto& levels = side == Side::BUY ? bidLevels_ : askLevels_;
        for (const auto& level : levels) {
            if (level.price == price) return level.totalQty;
        }
        return 0;
    }
};
// Using Map

class OrderBookMap {
private:
    struct OrderInfo {
        uint32_t price;
        uint32_t quantity;
        Side side;
    };
    
    unordered_map<uint64_t, OrderInfo> id2info_;
    map<uint32_t, PriceLevel> bidLevels_;
    map<uint32_t, PriceLevel> askLevels_;

public:
    OrderBookMap() {
        id2info_.reserve(100000);
    }
    
    void newOrder(const Order& order) {
        auto& levels = order.isBuy() ? bidLevels_ : askLevels_;
        auto& level = levels[order.price];
        if (level.price == 0) level.price = order.price;
        level.addOrder(order.quantity);
        id2info_[order.id] = {order.price, order.quantity, order.side};
    }
    
    void amendOrder(uint64_t orderId, uint32_t newQty) {
        auto it = id2info_.find(orderId);
        if (it == id2info_.end()) return;
        
        auto& info = it->second;
        auto& levels = info.side == Side::BUY ? bidLevels_ : askLevels_;
        auto levelIt = levels.find(info.price);
        
        if (levelIt != levels.end()) {
            levelIt->second.amendOrder(info.quantity, newQty);
            info.quantity = newQty;
            if (levelIt->second.isEmpty()) levels.erase(levelIt);
        }
    }
    
    void deleteOrder(uint64_t orderId) {
        auto it = id2info_.find(orderId);
        if (it == id2info_.end()) return;
        
        auto& info = it->second;
        auto& levels = info.side == Side::BUY ? bidLevels_ : askLevels_;
        auto levelIt = levels.find(info.price);
        
        if (levelIt != levels.end()) {
            levelIt->second.removeOrder(info.quantity);
            if (levelIt->second.isEmpty()) levels.erase(levelIt);
        }
        id2info_.erase(it);
    }
    
    TopOfBook topOfBook() const {
        TopOfBook tob;
        if (!bidLevels_.empty()) {
            auto it = bidLevels_.rbegin();
            tob.bidPrice = it->first;
            tob.bidQty = it->second.totalQty;
        }
        if (!askLevels_.empty()) {
            auto it = askLevels_.begin();
            tob.askPrice = it->first;
            tob.askQty = it->second.totalQty;
        }
        return tob;
    }
    
    size_t orderCount(uint32_t price, Side side) const {
        const auto& levels = side == Side::BUY ? bidLevels_ : askLevels_;
        auto it = levels.find(price);
        return it != levels.end() ? it->second.orderCount : 0;
    }
    
    uint64_t totalVolume(uint32_t price, Side side) const {
        const auto& levels = side == Side::BUY ? bidLevels_ : askLevels_;
        auto it = levels.find(price);
        return it != levels.end() ? it->second.totalQty : 0;
    }
};


// Using Heap

class OrderBookHeap {
private:
    struct OrderInfo {
        uint32_t price;
        uint32_t quantity;
        Side side;
    };
    
    unordered_map<uint64_t, OrderInfo> id2info_;
    map<uint32_t, PriceLevel> levels_;
    priority_queue<uint32_t> bidHeap_;
    priority_queue<uint32_t, vector<uint32_t>, greater<uint32_t>> askHeap_;

public:
    OrderBookHeap() {
        id2info_.reserve(100000);
    }
    
    void newOrder(const Order& order) {
        auto& level = levels_[order.price];
        if (level.price == 0) level.price = order.price;
        level.addOrder(order.quantity);
        
        if (order.isBuy()) bidHeap_.push(order.price);
        else askHeap_.push(order.price);
        
        id2info_[order.id] = {order.price, order.quantity, order.side};
    }
    
    void amendOrder(uint64_t orderId, uint32_t newQty) {
        auto it = id2info_.find(orderId);
        if (it == id2info_.end()) return;
        
        auto& info = it->second;
        auto levelIt = levels_.find(info.price);
        if (levelIt != levels_.end()) {
            levelIt->second.amendOrder(info.quantity, newQty);
            info.quantity = newQty;
        }
    }
    
    void deleteOrder(uint64_t orderId) {
        auto it = id2info_.find(orderId);
        if (it == id2info_.end()) return;
        
        auto& info = it->second;
        auto levelIt = levels_.find(info.price);
        if (levelIt != levels_.end()) {
            levelIt->second.removeOrder(info.quantity);
        }
        id2info_.erase(it);
    }
    
    TopOfBook topOfBook() const {
        TopOfBook tob;
        
        while (!bidHeap_.empty()) {
            uint32_t price = bidHeap_.top();
            auto it = levels_.find(price);
            if (it != levels_.end() && !it->second.isEmpty()) {
                tob.bidPrice = price;
                tob.bidQty = it->second.totalQty;
                break;
            }
            const_cast<priority_queue<uint32_t>&>(bidHeap_).pop();
        }
        
        while (!askHeap_.empty()) {
            uint32_t price = askHeap_.top();
            auto it = levels_.find(price);
            if (it != levels_.end() && !it->second.isEmpty()) {
                tob.askPrice = price;
                tob.askQty = it->second.totalQty;
                break;
            }
            const_cast<priority_queue<uint32_t, vector<uint32_t>, greater<uint32_t>>&>(askHeap_).pop();
        }
        
        return tob;
    }
    
    size_t orderCount(uint32_t price, Side) const {
        auto it = levels_.find(price);
        return it != levels_.end() ? it->second.orderCount : 0;
    }
    
    uint64_t totalVolume(uint32_t price, Side) const {
        auto it = levels_.find(price);
        return it != levels_.end() ? it->second.totalQty : 0;
    }
};


class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual void onTopOfBookUpdate(const TopOfBook& tob) = 0;
};

class SimpleStrategy : public IStrategy {
private:
    TopOfBook lastTob_;
    size_t updateCount_ = 0;
    
public:
    void onTopOfBookUpdate(const TopOfBook& tob) override {
        if (tob.bidPrice != lastTob_.bidPrice || tob.askPrice != lastTob_.askPrice) {
            lastTob_ = tob;
            updateCount_++;
        }
    }
    
    size_t getUpdateCount() const { return updateCount_; }
};

// Phase 3

enum class EventType { NEW, AMEND, DELETE };

struct Event {
    EventType type;
    Order order;
    uint32_t newQty;
};

vector<Event> generateEvents(size_t numEvents, uint64_t& nextOrderId) {
    vector<Event> events;
    events.reserve(numEvents);
    
    random_device rd;
    mt19937 gen(42); 
    uniform_int_distribution<> typeDist(0, 100);
    uniform_int_distribution<> priceDist(9900, 10100);
    uniform_int_distribution<> qtyDist(1, 1000);
    uniform_int_distribution<> sideDist(0, 1);
    
    vector<uint64_t> activeOrders;
    
    for (size_t i = 0; i < numEvents; ++i) {
        int typeRoll = typeDist(gen);
        
        if (typeRoll < 60 || activeOrders.empty()) {
            Event e;
            e.type = EventType::NEW;
            e.order = Order(nextOrderId++, priceDist(gen), qtyDist(gen),
                           sideDist(gen) == 0 ? Side::BUY : Side::SELL);
            events.push_back(e);
            activeOrders.push_back(e.order.id);
        } else if (typeRoll < 80) {
            uniform_int_distribution<> orderDist(0, activeOrders.size() - 1);
            Event e;
            e.type = EventType::AMEND;
            e.order.id = activeOrders[orderDist(gen)];
            e.newQty = qtyDist(gen);
            events.push_back(e);
        } else {
            uniform_int_distribution<> orderDist(0, activeOrders.size() - 1);
            size_t idx = orderDist(gen);
            Event e;
            e.type = EventType::DELETE;
            e.order.id = activeOrders[idx];
            events.push_back(e);
            activeOrders.erase(activeOrders.begin() + idx);
        }
    }
    
    return events;
}

struct BenchmarkResult {
    double totalTimeMs;
    double avgLatencyNs;
    double throughputMops;
    vector<double> latencies;
    
    void calculateStats() {
        sort(latencies.begin(), latencies.end());
    }
    
    double median() const {
        return latencies.empty() ? 0 : latencies[latencies.size() / 2];
    }
    
    double percentile(double p) const {
        if (latencies.empty()) return 0;
        size_t idx = static_cast<size_t>(latencies.size() * p / 100.0);
        return latencies[min(idx, latencies.size() - 1)];
    }
};

template<typename OrderBookType>
BenchmarkResult runBenchmark(const vector<Event>& events, bool measurePerOp = false) {
    BenchmarkResult result;
    OrderBookType book;
    
    if (measurePerOp) result.latencies.reserve(events.size());
    
    auto start = high_resolution_clock::now();
    
    for (const auto& event : events) {
        auto opStart = measurePerOp ? high_resolution_clock::now() : start;
        
        switch (event.type) {
            case EventType::NEW:
                book.newOrder(event.order);
                break;
            case EventType::AMEND:
                book.amendOrder(event.order.id, event.newQty);
                break;
            case EventType::DELETE:
                book.deleteOrder(event.order.id);
                break;
        }
        
        if (measurePerOp) {
            auto opEnd = high_resolution_clock::now();
            double latency = duration_cast<nanoseconds>(opEnd - opStart).count();
            result.latencies.push_back(latency);
        }
    }
    
    auto end = high_resolution_clock::now();
    
    result.totalTimeMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.avgLatencyNs = (result.totalTimeMs * 1e6) / events.size();
    result.throughputMops = events.size() / (result.totalTimeMs * 1000.0);
    
    if (measurePerOp) result.calculateStats();
    
    return result;
}

template<typename OrderBookType>
double benchmarkTopOfBook(OrderBookType& book, size_t iterations) {
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        volatile auto tob = book.topOfBook();
        (void)tob;
    }
    
    auto end = high_resolution_clock::now();
    double totalNs = duration_cast<nanoseconds>(end - start).count();
    return totalNs / iterations;
}

void printResults(const string& name, const BenchmarkResult& result, double tobLatency) {
    cout << "\n========== " << name << " ==========\n";
    cout << fixed << setprecision(2);
    cout << "Total Time:        " << result.totalTimeMs << " ms\n";
    cout << "Throughput:        " << result.throughputMops << " Mops/s\n";
    cout << "Avg Latency:       " << result.avgLatencyNs << " ns\n";
    
    if (!result.latencies.empty()) {
        cout << "Median Latency:    " << result.median() << " ns\n";
        cout << "90th Percentile:   " << result.percentile(90) << " ns\n";
        cout << "99th Percentile:   " << result.percentile(99) << " ns\n";
        cout << "99.9th Percentile: " << result.percentile(99.9) << " ns\n";
    }
    
    cout << "Top-of-Book Query: " << tobLatency << " ns\n";
}
// Unit Tests

void runUnitTests() {
    cout << "Running Unit Tests...\n";
    
    OrderBookMap book;
    

    book.newOrder(Order(1, 10000, 100, Side::BUY));
    book.newOrder(Order(2, 10010, 200, Side::SELL));
    
    auto tob = book.topOfBook();
    assert(tob.bidPrice == 10000);
    assert(tob.askPrice == 10010);

    book.amendOrder(1, 150);
    assert(book.totalVolume(10000, Side::BUY) == 150);
    

    book.deleteOrder(1);
    assert(book.orderCount(10000, Side::BUY) == 0);
    
    cout << "✓ All unit tests passed!\n\n";
}


// Main - Phase 6

int main() {
    cout << "========================================\n";
    cout << "Limit Order Book Performance Benchmark\n";
    cout << "========================================\n\n";
    
    runUnitTests();
    
    const size_t NUM_EVENTS = 10'000'000;
    const size_t TOB_QUERIES = 100'000;
    
    cout << "Generating " << NUM_EVENTS << " random events...\n";
    uint64_t nextOrderId = 1;
    auto events = generateEvents(NUM_EVENTS, nextOrderId);
    
    // Baseline
    cout << "\n[1/3] Testing Baseline (std::vector)...\n";
    auto resultBaseline = runBenchmark<OrderBookBaseline>(events, true);
    OrderBookBaseline bookBase;
    for (const auto& e : events) {
        if (e.type == EventType::NEW) bookBase.newOrder(e.order);
    }
    double tobBaseline = benchmarkTopOfBook(bookBase, TOB_QUERIES);
    printResults("Baseline (Vector)", resultBaseline, tobBaseline);
    
    // HashMap + std::map
    cout << "\n[2/3] Testing HashMap + std::map...\n";
    auto resultMap = runBenchmark<OrderBookMap>(events, true);
    OrderBookMap bookMap;
    for (const auto& e : events) {
        if (e.type == EventType::NEW) bookMap.newOrder(e.order);
    }
    double tobMap = benchmarkTopOfBook(bookMap, TOB_QUERIES);
    printResults("HashMap + std::map", resultMap, tobMap);
    
    // STL + Heaps
    cout << "\n[3/3] Testing STL + Heaps (Lazy Delete)...\n";
    auto resultHeap = runBenchmark<OrderBookHeap>(events, true);
    OrderBookHeap bookHeap;
    for (const auto& e : events) {
        if (e.type == EventType::NEW) bookHeap.newOrder(e.order);
    }
    double tobHeap = benchmarkTopOfBook(bookHeap, TOB_QUERIES);
    printResults("STL + Heaps", resultHeap, tobHeap);
    

    cout << "Performance Comparison Table\n";

    cout << left << setw(25) << "Implementation"
         << right << setw(15) << "Throughput"
         << setw(15) << "Avg Latency"
         << setw(15) << "ToB Query" << "\n";
    cout << left << setw(25) << ""
         << right << setw(15) << "(Mops/s)"
         << setw(15) << "(ns)"
         << setw(15) << "(ns)" << "\n";
    cout << string(70, '-') << "\n";
    
    cout << left << setw(25) << "Baseline Vector"
         << right << setw(15) << resultBaseline.throughputMops
         << setw(15) << resultBaseline.avgLatencyNs
         << setw(15) << tobBaseline << "\n";
    
    cout << left << setw(25) << "HashMap + std::map"
         << right << setw(15) << resultMap.throughputMops
         << setw(15) << resultMap.avgLatencyNs
         << setw(15) << tobMap << "\n";
    
    cout << left << setw(25) << "STL + Heaps"
         << right << setw(15) << resultHeap.throughputMops
         << setw(15) << resultHeap.avgLatencyNs
         << setw(15) << tobHeap << "\n";
    

    cout << "Benchmark Complete!\n";
    
    return 0;
}