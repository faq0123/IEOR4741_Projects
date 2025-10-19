#pragma once
#include <string>
#include <memory>
#include <type_traits>

template <typename PriceType, typename OrderIdType>
struct Order {
    // Compile-time check: OrderIdType must be integral
    static_assert(std::is_integral<OrderIdType>::value, "Order ID must be an integer");
    static_assert(std::is_arithmetic<PriceType>::value, "Price must be an arithmetic type");

    OrderIdType id;
    std::string symbol;
    PriceType price;
    int quantity;
    bool is_buy;
    std::chrono::high_resolution_clock::time_point timestamp;

    // Default constructor (needed for memory pool)
    Order() : id(0), symbol(""), price(0), quantity(0), is_buy(false),
              timestamp(std::chrono::high_resolution_clock::now()) {}

    Order(OrderIdType id, std::string sym, PriceType pr, int qty, bool buy)
        : id(id), symbol(std::move(sym)), price(pr), quantity(qty), is_buy(buy),
          timestamp(std::chrono::high_resolution_clock::now()) {}

    // Copy constructor
    Order(const Order& other) = default;
    
    // Move constructor
    Order(Order&& other) noexcept = default;

    // Assignment operators
    Order& operator=(const Order& other) = default;
    Order& operator=(Order&& other) noexcept = default;

    ~Order() = default;
};
