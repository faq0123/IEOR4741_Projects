#include "market_snapshot.h"
#include "order_manager.h"
#include "feed_parser.h"

#include <iostream>
using namespace std;

int simple_strategy(const MarketSnapshot& snap, int trade_qty = 50) {
    const PriceLevel* bb = snap.get_best_bid();
    const PriceLevel* ba = snap.get_best_ask();
    if (!bb || !ba) return 0;

    double spread = ba->price - bb->price;

    if (spread <= 0.02) {
        return trade_qty; // buy
    }

    if (spread >= 0.1) {
        return -trade_qty; // sell
    }

    return 0; // no action
}

int main() {
    MarketSnapshot snapshot;
    OrderManager om;

    const double EPS = 1e-9;
    double last_buy_price  = std::numeric_limits<double>::quiet_NaN(); 
    double last_sell_price = std::numeric_limits<double>::quiet_NaN(); 

    auto feed = load_feed("sample_feed.txt");

    for (const auto& event : feed) {
        // event.print();

        // 1) Update market events
        if (event.type == FeedType::BID) {
            snapshot.update_bid(event.price, event.quantity);
        } else if (event.type == FeedType::ASK) {
            snapshot.update_ask(event.price, event.quantity);
        } else if (event.type == FeedType::EXECUTION) {
            om.handle_fill(event.order_id, event.quantity);
        }

        // 2) Strategy Decision
        int decision = simple_strategy(snapshot);
        if (decision > 0) { // Buy
            const PriceLevel* ba = snapshot.get_best_ask();
            bool same_buy = (!isnan(last_buy_price)) && (fabs(ba->price - last_buy_price) < EPS);
            if (!same_buy) {
                om.place_order(Side::Buy, ba->price, decision);
                last_buy_price = ba->price;
            }
        } else if (decision < 0) { // Sell
            const PriceLevel* bb = snapshot.get_best_bid();
            bool same_sell = (!isnan(last_sell_price)) && (fabs(bb->price - last_sell_price) < EPS);
            if (!same_sell) {
                om.place_order(Side::Sell, bb->price, -decision);
                last_sell_price = bb->price;
            }
        }
    }

    cout << "---- Trading Session End ----\n";
    om.print_active_orders();
    return 0;
}