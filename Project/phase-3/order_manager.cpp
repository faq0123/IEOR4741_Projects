#include "order_manager.h"
#include <iostream>

int OrderManager::place_order(Side side, double price, int qty) {
    const int id = next_id++;
    auto ord = std::make_unique<MyOrder>(MyOrder{ id, side, price, qty });
    orders.emplace(id, std::move(ord));

    std::cout << "[Strategy] Placing " << (side == Side::Buy ? "BUY" : "SELL")
         << " order at " << price << " x " << qty << " (ID = " << id << ")\n";
    return id;
}

void OrderManager::cancel(int id) {
    auto it = orders.find(id);

    // When an order is filled or canceled, simply remove it from the map
    if (it == orders.end()) {
        std::cout << "[ERROR] Order " << id << " not found, cannot be cancelled.\n";
    } else if (it->second->status == OrderStatus::Cancelled) {
        std::cout << "[ERROR] Order " << id << " is already cancelled.\n";
    } else if (it->second->status == OrderStatus::Filled) {
        std::cout << "[ERROR] Order " << id << " is already filled, cannot be cancelled.\n";
    } else {
        it->second->status = OrderStatus::Cancelled;
        std::cout << "[Order] order " << id << " is Cancelled\n";
        orders.erase(it);
    }
}

void OrderManager::handle_fill(int id, int filled_qty) {
    auto it = orders.find(id);
    if (it == orders.end()) {
        std::cout << "[ERROR] Order " << id << " not found, cannot process fill.\n";
        return;
    }

    MyOrder& order = *it->second;
    if (order.status == OrderStatus::Cancelled) {
        std::cout << "[ERROR] Order " << id << " is cancelled, cannot process fill.\n";
        return;
    }
    if (order.status == OrderStatus::Filled) {
        std::cout << "[ERROR] Order " << id << " is already filled.\n";
        return;
    }

    const int remaining = order.quantity - order.filled;
    const int inc = std::min(remaining, filled_qty);
    order.filled += inc;

    if (order.filled >= order.quantity) {
        order.status = OrderStatus::Filled;
        std::cout << "[Order] Order " << id << " fully filled (" 
             << order.filled << "/" << order.quantity << ") and removed\n";
        orders.erase(it); // remove filled orders
    } else {
        order.status = OrderStatus::PartiallyFilled;
        std::cout << "[Order] Order " << id << " partially filled: " << order.filled << "/" << order.quantity << "\n";
    }

}

const MyOrder* OrderManager::get(int id) const {
    auto it = orders.find(id);
    return it == orders.end() ? nullptr : it->second.get();
}

void OrderManager::print_active_orders() const {
    if (orders.empty()) {
        std::cout << "[OrderManager] No active orders.\n";
        return;
    }

    std::cout << "Active Orders:\n";
    for (const auto& pair : orders) {
        const MyOrder& order = *pair.second;
        if (order.status != OrderStatus::Cancelled && order.status != OrderStatus::Filled) {
            std::cout << "Order " << order.id << ": "
                 << (order.side == Side::Buy ? "BUY" : "SELL") << " "
                 << order.quantity << " @ " << order.price
                 << " (Filled: " << order.filled << ", Status: ";
            switch (order.status) {
                case OrderStatus::New: std::cout << "New"; break;
                case OrderStatus::PartiallyFilled: std::cout << "Partially Filled"; break;
                default: std::cout << "Unknown"; break;
            }
            std::cout << ")\n";
        }
    }
}