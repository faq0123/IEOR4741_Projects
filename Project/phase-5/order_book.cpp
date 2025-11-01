// g++ -O3 -std=c++11 orderbook_bench.cpp -o ob && ./ob
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <chrono>
#include <cassert>
#include <random>
#include <vector>
#include <atomic>
#include <iomanip>
#include <type_traits>

// ------------ branch prediction helpers (portable fallbacks) ------------
#if defined(__GNUC__) || defined(__clang__)
  #define LIKELY(x)   (__builtin_expect(!!(x), 1))
  #define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
  #define LIKELY(x)   (x)
  #define UNLIKELY(x) (x)
#endif

// ------------ Order struct ------------
struct Order {
    std::string id;   // String order ID
    double      price;
    int         quantity;
    bool        isBuy;
};

// ======================================================================
// Baseline: duplicates Order in both (price level container) and (lookup)
// price -> (orderId -> Order), and orderId -> Order
// ======================================================================
class BaselineOrderBook {
private:
    std::map<double, std::unordered_map<std::string, Order> > orderLevels;
    std::unordered_map<std::string, Order> orderLookup;

public:
    BaselineOrderBook() {
        orderLookup.max_load_factor(0.70f);
        orderLookup.reserve(1 << 15);
    }

    void addOrder(const std::string& id, double price, int quantity, bool isBuy) {
        Order ord;
        ord.id = id;
        ord.price = price;
        ord.quantity = quantity;
        ord.isBuy = isBuy;

        std::unordered_map<std::string, Order>& level = orderLevels[price];
        if (level.bucket_count() == 0) {
            level.max_load_factor(0.70f);
            level.reserve(8);
        }
        level.emplace(id, ord);
        orderLookup.emplace(id, ord);
    }

    void modifyOrder(const std::string& id, double newPrice, int newQuantity) {
        std::unordered_map<std::string, Order>::iterator it = orderLookup.find(id);
        if (UNLIKELY(it == orderLookup.end())) return;

        const Order& old = it->second;
        std::map<double, std::unordered_map<std::string, Order> >::iterator levelIt = orderLevels.find(old.price);
        if (LIKELY(levelIt != orderLevels.end())) {
            levelIt->second.erase(id);
            if (UNLIKELY(levelIt->second.empty()))
                orderLevels.erase(levelIt);
        }
        addOrder(id, newPrice, newQuantity, old.isBuy);
    }

    void deleteOrder(const std::string& id) {
        std::unordered_map<std::string, Order>::iterator it = orderLookup.find(id);
        if (UNLIKELY(it == orderLookup.end())) return;

        const Order& ord = it->second;
        std::map<double, std::unordered_map<std::string, Order> >::iterator levelIt = orderLevels.find(ord.price);
        if (LIKELY(levelIt != orderLevels.end())) {
            levelIt->second.erase(id);
            if (UNLIKELY(levelIt->second.empty()))
                orderLevels.erase(levelIt);
        }
        orderLookup.erase(it);
    }

    bool hasOrder(const std::string& id) const {
        return orderLookup.find(id) != orderLookup.end();
    }
    std::size_t orderCount() const { return orderLookup.size(); }
    std::size_t levelCount() const { return orderLevels.size(); }
};

// ======================================================================
// Optimized: single source of truth for Order (in one unordered_map)
// price levels keep only order IDs (unordered_set<std::string>).
// ======================================================================
class OptimizedOrderBook {
private:
    std::unordered_map<std::string, Order> orderStore;
    std::map<double, std::unordered_set<std::string> > levelIds;
    std::atomic<int> orderCountAtomic;

public:
    OptimizedOrderBook() : orderCountAtomic(0) {
        orderStore.max_load_factor(0.70f);
        orderStore.reserve(1 << 17);
    }

    void addOrder(const std::string& id, double price, int quantity, bool isBuy) {
        std::pair< std::unordered_map<std::string, Order>::iterator, bool > res =
            orderStore.emplace(id, Order());

        Order& o = res.first->second;
        if (res.second) {
            o.id = id;
        } else {
            removeFromLevel(o.price, id);
        }
        o.price = price;
        o.quantity = quantity;
        o.isBuy = isBuy;

        std::unordered_set<std::string>& ids = levelIds[price];
        if (ids.bucket_count() == 0) {
            ids.max_load_factor(0.70f);
            ids.reserve(8);
        }
        ids.insert(id);

        orderCountAtomic.fetch_add(1, std::memory_order_relaxed);
    }

    void modifyOrder(const std::string& id, double newPrice, int newQuantity) {
        std::unordered_map<std::string, Order>::iterator it = orderStore.find(id);
        if (UNLIKELY(it == orderStore.end())) return;

        Order& o = it->second;
        if (newPrice != o.price) {
            removeFromLevel(o.price, id);
            std::unordered_set<std::string>& ids = levelIds[newPrice];
            if (ids.bucket_count() == 0) {
                ids.max_load_factor(0.70f);
                ids.reserve(8);
            }
            ids.insert(id);
            o.price = newPrice;
        }
        o.quantity = newQuantity;
    }

    void deleteOrder(const std::string& id) {
        std::unordered_map<std::string, Order>::iterator it = orderStore.find(id);
        if (UNLIKELY(it == orderStore.end())) return;

        removeFromLevel(it->second.price, id);
        orderStore.erase(it);
    }

    bool hasOrder(const std::string& id) const {
        return orderStore.find(id) != orderStore.end();
    }
    std::size_t orderCount() const { return orderStore.size(); }
    std::size_t levelCount() const { return levelIds.size(); }
    int approxOrderCount() const { return orderCountAtomic.load(std::memory_order_relaxed); }

private:
    inline void removeFromLevel(double price, const std::string& id) {
        std::map<double, std::unordered_set<std::string> >::iterator it = levelIds.find(price);
        if (LIKELY(it != levelIds.end())) {
            std::unordered_set<std::string>& ids = it->second;
            ids.erase(id);
            if (UNLIKELY(ids.empty()))
                levelIds.erase(it);
        }
    }
};

// ======================================================================
// Tests
// ======================================================================
template <typename Book>
void testAddOrder() {
    Book book;
    book.addOrder("ORD001", 50.10, 100, true);
    assert(book.hasOrder("ORD001"));
    assert(book.orderCount() == 1);
}
template <typename Book>
void testModifyOrder() {
    Book book;
    book.addOrder("ORD001", 50.10, 100, true);
    book.modifyOrder("ORD001", 50.15, 120);
    assert(book.hasOrder("ORD001"));
    assert(book.orderCount() == 1);
}
template <typename Book>
void testDeleteOrder() {
    Book book;
    book.addOrder("ORD001", 50.10, 100, true);
    book.deleteOrder("ORD001");
    assert(!book.hasOrder("ORD001"));
    assert(book.orderCount() == 0);
}

// ======================================================================
// Stress helpers
// ======================================================================
template <typename Book>
void stressInsert(Book& book, int numOrders, uint32_t seed = std::random_device()()) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> priceDist(50.0, 100.0);
    std::uniform_int_distribution<int> quantityDist(1, 500);
    std::bernoulli_distribution sideDist(0.5);

    for (int i = 0; i < numOrders; ++i) {
        std::string id = "ORD" + std::to_string(i);
        double price = priceDist(rng);
        int quantity = quantityDist(rng);
        bool isBuy = sideDist(rng);
        book.addOrder(id, price, quantity, isBuy);
    }
}

template <typename Book>
void stressMixed(Book& book, int numOrders, int numModify, int numDelete, uint32_t seed = std::random_device()()) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> priceDist(50.0, 100.0);
    std::uniform_int_distribution<int> quantityDist(1, 500);
    for (int i = 0; i < numModify; ++i) {
        std::string id = "ORD" + std::to_string(i);
        book.modifyOrder(id, priceDist(rng), quantityDist(rng));
    }
    for (int i = numModify; i < numModify + numDelete && i < numOrders; ++i) {
        std::string id = "ORD" + std::to_string(i);
        book.deleteOrder(id);
    }
}

using OrderBook = BaselineOrderBook;

template <typename Book>
inline void stressTest(Book& book, int N) { stressInsert(book, N); }

// Loop-unrolling demo
template <typename Book>
void processOrders_unroll2(std::vector<Order>& orders) {
    (void)sizeof(Book);
    const size_t n = orders.size();
    size_t i = 0;
    for (; i + 1 < n; i += 2) {
        (void)orders[i];
        (void)orders[i + 1];
    }
    if (i < n) (void)orders[i];
}

// Micro-benchmark
template <typename Book>
void microBenchmark(Book& book, const char* name) {
    std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();

    book.addOrder("ORD001", 50.10, 100, true);
    book.addOrder("ORD002", 50.20, 150, false);
    book.modifyOrder("ORD001", 50.15, 120);
    book.deleteOrder("ORD002");

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> el = t1 - t0;

    std::cout << std::fixed << std::setprecision(6)
              << name << " micro-bench: " << el.count() << " s\n";
}

// Bench helper
template <typename Book>
double benchOnce(int N) {
    Book book;
    auto t0 = std::chrono::high_resolution_clock::now();
    stressInsert(book, N);
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    std::cout << "  orders=" << N
              << " | levels=" << book.levelCount()
              << " | orders=" << book.orderCount()
              << " | time=" << dt.count() << " s\n";
    return dt.count();
}

int main() {
    // Unit tests
    testAddOrder<OrderBook>();
    testModifyOrder<OrderBook>();
    testDeleteOrder<OrderBook>();
    testAddOrder<OptimizedOrderBook>();
    testModifyOrder<OptimizedOrderBook>();
    testDeleteOrder<OptimizedOrderBook>();
    std::cout << "[UnitTests] OK\n";

    // Benchmarks
    std::vector<int> sizes   = {1000, 5000, 10000, 50000, 100000};
    int trials = 5;

    std::cout << "\n=== OrderBook Benchmarks ===\n";
    for (int n : sizes) {
        double sum = 0.0;
        for (int t = 0; t < trials; ++t) {
            sum += benchOnce<OrderBook>(n);
        }
        double avg = sum / trials;
        std::cout << "[AVG] orders=" << n
                  << " | avg=" << std::fixed << std::setprecision(6) << avg << " s"
                  << " | " << (avg * 1e9 / n) << " ns/op\n";
    }

    std::cout << "\n=== OptimizedOrderBook Benchmarks ===\n";
    for (int n : sizes) {
        double sum = 0.0;
        for (int t = 0; t < trials; ++t) {
            sum += benchOnce<OptimizedOrderBook>(n);
        }
        double avg = sum / trials;
        std::cout << "[AVG] orders=" << n
                  << " | avg=" << std::fixed << std::setprecision(6) << avg << " s"
                  << " | " << (avg * 1e9 / n) << " ns/op\n";
    }

    // micro + unroll demo
    {
        OrderBook book;
        microBenchmark(book, "OrderBook        ");
    }
    {
        OptimizedOrderBook book;
        microBenchmark(book, "OptimizedOrderBook");
    }

    std::vector<Order> batch;
    batch.reserve(5);
    batch.push_back({"ORDX1", 50.01, 10, true});
    batch.push_back({"ORDX2", 50.02, 20, false});
    batch.push_back({"ORDX3", 50.03, 30, true});
    processOrders_unroll2<OrderBook>(batch);

    return 0;
}
