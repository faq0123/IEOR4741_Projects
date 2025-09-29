#ifndef MARKET_SNAPSHOT_H
#define MARKET_SNAPSHOT_H

#include <map>
#include <memory>
#include <limits>   // std::numeric_limits<double>::quiet_NaN()

// 价位节点
struct PriceLevel {
    double price{};
    int    quantity{};

    PriceLevel(double p, int q) : price(p), quantity(q) {}
};

// for descending order in bids map
struct Descend {
    bool operator()(double a, double b) const {
        return a > b;
    }
};

class MarketSnapshot {
public:
    void update_bid(double price, int qty);
    void update_ask(double price, int qty);

    const PriceLevel* get_best_bid() const;
    const PriceLevel* get_best_ask() const;

    // if no bid/ask, return NaN
    double best_bid_price() const; 
    double best_ask_price() const;

    bool has_bid() const { return !bids.empty(); }
    bool has_ask() const { return !asks.empty(); }
    void clear();

private:
    std::map<double, std::unique_ptr<PriceLevel>, Descend> bids; // sorted descending
    std::map<double, std::unique_ptr<PriceLevel>>          asks; // sorted ascending
};

#endif // MARKET_SNAPSHOT_H
