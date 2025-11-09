#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <sstream>
#include <iostream>
#include <ctime>

// Trading order structure
struct Order {
    std::string type;     // Order type: BUY or SELL
    std::string symbol;   // Stock symbol
    int quantity;         // Share quantity
    double price;         // Price per share
    std::string timestamp;// Order timestamp
    
    Order() : quantity(0), price(0.0) {}
    
    Order(const std::string& t, const std::string& s, int q, double p) 
        : type(t), symbol(s), quantity(q), price(p) {
        timestamp = getCurrentTime();
    }
    
    // Get current timestamp as string
    static std::string getCurrentTime() {
        time_t now = time(0);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }
    
    // Convert order to display string
    std::string toString() const {
        std::ostringstream oss;
        oss << type << " " << symbol << " " << quantity << " @ " << price;
        return oss.str();
    }
};

// Parse order string into Order struct
// Format: "ORDER_TYPE SYMBOL QUANTITY PRICE"
// Returns true if parsing succeeds
inline bool parseOrder(const std::string& message, Order& order) {
    std::istringstream iss(message);
    std::string type, symbol;
    int quantity;
    double price;
    
    if (!(iss >> type >> symbol >> quantity >> price)) {
        return false;
    }
    
    // Validate order type
    if (type != "BUY" && type != "SELL") {
        return false;
    }
    
    // Validate positive values
    if (quantity <= 0 || price <= 0.0) {
        return false;
    }
    
    order.type = type;
    order.symbol = symbol;
    order.quantity = quantity;
    order.price = price;
    order.timestamp = Order::getCurrentTime();
    
    return true;
}

#endif // ORDER_H
