#include "market_snapshot.h"
#include <iostream>

void MarketSnapshot::update_bid(double price, int qty) {
    double old_best = best_bid_price();

    if (qty <= 0) {
        if (price == old_best) {
            std::cout << "[Market] Best Bid: " << price << " removed\n";
        }

        bids.erase(price);

        if (bids.empty()) {
            std::cout << "[Market] No more bids\n";
        }
        return;
    } else {
        auto& slot = bids[price];
        if (!slot) slot = std::make_unique<PriceLevel>(PriceLevel{price, qty});
        else       slot->quantity = qty;
    }

    double new_best = best_bid_price();

    if (old_best != new_best || price == new_best) {
        const PriceLevel* bb = get_best_bid();
        std::cout << "[Market] Best Bid: " << bb->price
                  << " x " << bb->quantity << "\n";
    }
}

void MarketSnapshot::update_ask(double price, int qty) {
    double old_best = best_ask_price();

    if (qty <= 0) {
        if (price == old_best) {
            std::cout << "[Market] Best Ask: " << price << " removed \n";
        }

        asks.erase(price);

        if (asks.empty()) {
            std::cout << "[Market] No more asks\n";
        }
        return;
    } else {
        auto& slot = asks[price];
        if (!slot) slot = std::make_unique<PriceLevel>(PriceLevel{price, qty});
        else       slot->quantity = qty;
    }

    double new_best = best_ask_price();

    if (old_best != new_best || price == new_best) {
        const PriceLevel* ba = get_best_ask();
        std::cout << "[Market] Best Ask: " << ba->price
                  << " x " << ba->quantity << "\n";
    }
}

const PriceLevel* MarketSnapshot::get_best_bid() const {
    if (bids.empty()) return nullptr;
    return bids.begin()->second.get(); // highest price
}

const PriceLevel* MarketSnapshot::get_best_ask() const {
    if (asks.empty()) return nullptr;
    return asks.begin()->second.get(); // lowest price
}

double MarketSnapshot::best_bid_price() const {
    const PriceLevel* p = get_best_bid();
    return p ? p->price : std::numeric_limits<double>::quiet_NaN();
}

double MarketSnapshot::best_ask_price() const {
    const PriceLevel* p = get_best_ask();
    return p ? p->price : std::numeric_limits<double>::quiet_NaN();
}

void MarketSnapshot::clear() {
    bids.clear();
    asks.clear();
}