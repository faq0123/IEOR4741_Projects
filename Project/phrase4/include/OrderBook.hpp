#pragma once
#include "Order.hpp"
#include <map>
#include <memory>
#include <vector>
#include <algorithm>

// Memory pool allocator for performance optimization
template<typename T>
class MemoryPool {
private:
    std::vector<std::unique_ptr<T[]>> blocks;
    std::vector<T*> free_list;
    size_t block_size;
    size_t current_block_index;
    size_t current_offset;

public:
    explicit MemoryPool(size_t block_sz = 1024) 
        : block_size(block_sz), current_block_index(0), current_offset(0) {
        allocate_block();
    }

    T* allocate() {
        if (!free_list.empty()) {
            T* ptr = free_list.back();
            free_list.pop_back();
            return ptr;
        }

        if (current_offset >= block_size) {
            allocate_block();
        }

        return &blocks[current_block_index][current_offset++];
    }

    void deallocate(T* ptr) {
        free_list.push_back(ptr);
    }

private:
    void allocate_block() {
        blocks.push_back(std::make_unique<T[]>(block_size));
        current_block_index = blocks.size() - 1;
        current_offset = 0;
    }
};

// Template-based Order Book
template <typename PriceType, typename OrderIdType>
class OrderBook {
public:
    using OrderType = Order<PriceType, OrderIdType>;
    using OrderPtr = std::unique_ptr<OrderType>;

private:
    // Buy orders: higher price has priority (max heap behavior)
    // Use greater for descending order
    std::multimap<PriceType, OrderPtr, std::greater<PriceType>> buy_orders;
    
    // Sell orders: lower price has priority (min heap behavior)
    std::multimap<PriceType, OrderPtr, std::less<PriceType>> sell_orders;
    
    std::string symbol;
    MemoryPool<OrderType> memory_pool;

public:
    explicit OrderBook(const std::string& sym) : symbol(sym), memory_pool(1024) {}

    // Add a buy order
    void addBuyOrder(OrderPtr order) {
        if (order && order->is_buy) {
            buy_orders.emplace(order->price, std::move(order));
        }
    }

    // Add a sell order
    void addSellOrder(OrderPtr order) {
        if (order && !order->is_buy) {
            sell_orders.emplace(order->price, std::move(order));
        }
    }

    // Add order (automatically determines buy/sell)
    void addOrder(OrderPtr order) {
        if (!order) return;
        
        if (order->is_buy) {
            addBuyOrder(std::move(order));
        } else {
            addSellOrder(std::move(order));
        }
    }

    // Get best bid (highest buy price)
    PriceType getBestBid() const {
        if (buy_orders.empty()) return PriceType{};
        return buy_orders.begin()->first;
    }

    // Get best ask (lowest sell price)
    PriceType getBestAsk() const {
        if (sell_orders.empty()) return PriceType{};
        return sell_orders.begin()->first;
    }

    // Check if orders can be matched
    bool canMatch() const {
        if (buy_orders.empty() || sell_orders.empty()) return false;
        return getBestBid() >= getBestAsk();
    }

    // Get top buy orders
    std::vector<const OrderType*> getTopBuyOrders(size_t count = 5) const {
        std::vector<const OrderType*> result;
        result.reserve(count);
        
        auto it = buy_orders.begin();
        for (size_t i = 0; i < count && it != buy_orders.end(); ++i, ++it) {
            result.push_back(it->second.get());
        }
        return result;
    }

    // Get top sell orders
    std::vector<const OrderType*> getTopSellOrders(size_t count = 5) const {
        std::vector<const OrderType*> result;
        result.reserve(count);
        
        auto it = sell_orders.begin();
        for (size_t i = 0; i < count && it != sell_orders.end(); ++i, ++it) {
            result.push_back(it->second.get());
        }
        return result;
    }

    // Remove and return the best buy order
    OrderPtr popBestBuy() {
        if (buy_orders.empty()) return nullptr;
        
        auto it = buy_orders.begin();
        OrderPtr order = std::move(it->second);
        buy_orders.erase(it);
        return order;
    }

    // Remove and return the best sell order
    OrderPtr popBestSell() {
        if (sell_orders.empty()) return nullptr;
        
        auto it = sell_orders.begin();
        OrderPtr order = std::move(it->second);
        sell_orders.erase(it);
        return order;
    }

    // Get statistics
    size_t getBuyOrderCount() const { return buy_orders.size(); }
    size_t getSellOrderCount() const { return sell_orders.size(); }
    size_t getTotalOrderCount() const { return buy_orders.size() + sell_orders.size(); }

    const std::string& getSymbol() const { return symbol; }

    // Clear all orders
    void clear() {
        buy_orders.clear();
        sell_orders.clear();
    }
};
