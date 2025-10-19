#pragma once
#include "Order.hpp"
#include <unordered_map>
#include <memory>
#include <string>

// Order states for tracking
enum class OrderState {
    NEW,
    PARTIAL_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

// Order info for management
template <typename PriceType, typename OrderIdType>
struct OrderInfo {
    OrderIdType id;
    std::string symbol;
    PriceType price;
    int original_quantity;
    int remaining_quantity;
    bool is_buy;
    OrderState state;
    std::chrono::high_resolution_clock::time_point created_at;
    std::chrono::high_resolution_clock::time_point updated_at;

    OrderInfo(const Order<PriceType, OrderIdType>& order)
        : id(order.id), symbol(order.symbol), price(order.price),
          original_quantity(order.quantity), remaining_quantity(order.quantity),
          is_buy(order.is_buy), state(OrderState::NEW),
          created_at(order.timestamp), updated_at(order.timestamp) {}
};

// Order Management System
template <typename PriceType, typename OrderIdType>
class OrderManager {
public:
    using OrderType = Order<PriceType, OrderIdType>;
    using OrderPtr = std::unique_ptr<OrderType>;
    using OrderInfoType = OrderInfo<PriceType, OrderIdType>;

private:
    std::unordered_map<OrderIdType, std::shared_ptr<OrderInfoType>> orders;
    OrderIdType next_order_id;

public:
    OrderManager() : next_order_id(1) {}

    // Create and register a new order
    OrderPtr createOrder(const std::string& symbol, PriceType price, 
                        int quantity, bool is_buy) {
        OrderIdType id = next_order_id++;
        auto order = std::make_unique<OrderType>(id, symbol, price, quantity, is_buy);
        
        // Register the order
        auto order_info = std::make_shared<OrderInfoType>(*order);
        orders[id] = order_info;

        return order;
    }

    // Update order state
    void updateOrderState(OrderIdType id, OrderState state) {
        auto it = orders.find(id);
        if (it != orders.end()) {
            it->second->state = state;
            it->second->updated_at = std::chrono::high_resolution_clock::now();
        }
    }

    // Update remaining quantity (for partial fills)
    void updateRemainingQuantity(OrderIdType id, int remaining) {
        auto it = orders.find(id);
        if (it != orders.end()) {
            it->second->remaining_quantity = remaining;
            it->second->updated_at = std::chrono::high_resolution_clock::now();
            
            // Update state based on remaining quantity
            if (remaining == 0) {
                it->second->state = OrderState::FILLED;
            } else if (remaining < it->second->original_quantity) {
                it->second->state = OrderState::PARTIAL_FILLED;
            }
        }
    }

    // Cancel an order
    bool cancelOrder(OrderIdType id) {
        auto it = orders.find(id);
        if (it != orders.end() && it->second->state != OrderState::FILLED) {
            it->second->state = OrderState::CANCELLED;
            it->second->updated_at = std::chrono::high_resolution_clock::now();
            return true;
        }
        return false;
    }

    // Get order info
    std::shared_ptr<OrderInfoType> getOrderInfo(OrderIdType id) const {
        auto it = orders.find(id);
        return (it != orders.end()) ? it->second : nullptr;
    }

    // Get all orders
    const std::unordered_map<OrderIdType, std::shared_ptr<OrderInfoType>>& getAllOrders() const {
        return orders;
    }

    // Get statistics
    size_t getTotalOrders() const { return orders.size(); }

    size_t getOrdersByState(OrderState state) const {
        size_t count = 0;
        for (const auto& pair : orders) {
            if (pair.second->state == state) {
                ++count;
            }
        }
        return count;
    }

    // Convert order state to string
    static std::string stateToString(OrderState state) {
        switch (state) {
            case OrderState::NEW: return "NEW";
            case OrderState::PARTIAL_FILLED: return "PARTIAL_FILLED";
            case OrderState::FILLED: return "FILLED";
            case OrderState::CANCELLED: return "CANCELLED";
            case OrderState::REJECTED: return "REJECTED";
            default: return "UNKNOWN";
        }
    }
};
