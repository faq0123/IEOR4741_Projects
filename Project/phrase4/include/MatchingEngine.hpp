#pragma once
#include "Order.hpp"
#include "OrderBook.hpp"
#include <vector>
#include <memory>
#include <chrono>

// Trade structure to record matched trades
template <typename PriceType, typename OrderIdType>
struct Trade {
    OrderIdType buy_order_id;
    OrderIdType sell_order_id;
    std::string symbol;
    PriceType price;
    int quantity;
    std::chrono::high_resolution_clock::time_point timestamp;

    Trade(OrderIdType buy_id, OrderIdType sell_id, std::string sym, 
          PriceType pr, int qty)
        : buy_order_id(buy_id), sell_order_id(sell_id), 
          symbol(std::move(sym)), price(pr), quantity(qty),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

// High-performance matching engine
template <typename PriceType, typename OrderIdType>
class MatchingEngine {
public:
    using OrderType = Order<PriceType, OrderIdType>;
    using OrderPtr = std::unique_ptr<OrderType>;
    using TradeType = Trade<PriceType, OrderIdType>;
    using OrderBookType = OrderBook<PriceType, OrderIdType>;

private:
    OrderBookType& order_book;
    std::vector<TradeType> trades;

public:
    explicit MatchingEngine(OrderBookType& book) : order_book(book) {
        trades.reserve(10000); // Pre-allocate for performance
    }

    // Match a single order against the book
    std::vector<TradeType> matchOrder(OrderPtr order) {
        std::vector<TradeType> matched_trades;
        
        if (!order) return matched_trades;

        if (order->is_buy) {
            // Match buy order against sell orders
            matchBuyOrder(std::move(order), matched_trades);
        } else {
            // Match sell order against buy orders
            matchSellOrder(std::move(order), matched_trades);
        }

        // Add to global trade history
        trades.insert(trades.end(), matched_trades.begin(), matched_trades.end());

        return matched_trades;
    }

    // Continuously match orders in the book
    std::vector<TradeType> matchAll() {
        std::vector<TradeType> matched_trades;

        while (order_book.canMatch()) {
            OrderPtr buy_order = order_book.popBestBuy();
            OrderPtr sell_order = order_book.popBestSell();

            if (!buy_order || !sell_order) break;

            // Execute trade at the sell price (typically in real markets)
            PriceType trade_price = sell_order->price;
            int trade_quantity = std::min(buy_order->quantity, sell_order->quantity);

            TradeType trade(buy_order->id, sell_order->id, buy_order->symbol,
                          trade_price, trade_quantity);
            
            matched_trades.push_back(trade);

            // Update order quantities
            buy_order->quantity -= trade_quantity;
            sell_order->quantity -= trade_quantity;

            // Re-add partially filled orders
            if (buy_order->quantity > 0) {
                order_book.addBuyOrder(std::move(buy_order));
            }
            if (sell_order->quantity > 0) {
                order_book.addSellOrder(std::move(sell_order));
            }
        }

        // Add to global trade history
        trades.insert(trades.end(), matched_trades.begin(), matched_trades.end());

        return matched_trades;
    }

    const std::vector<TradeType>& getTrades() const { return trades; }
    
    size_t getTradeCount() const { return trades.size(); }

    void clearTrades() { trades.clear(); }

private:
    void matchBuyOrder(OrderPtr buy_order, std::vector<TradeType>& matched_trades) {
        while (buy_order && buy_order->quantity > 0) {
            // Check if we can match
            if (order_book.getSellOrderCount() == 0) {
                // No sell orders, add buy order to book
                order_book.addBuyOrder(std::move(buy_order));
                break;
            }

            PriceType best_ask = order_book.getBestAsk();
            
            // Check if price matches
            if (buy_order->price < best_ask) {
                // Price doesn't match, add to book
                order_book.addBuyOrder(std::move(buy_order));
                break;
            }

            // Match with best sell order
            OrderPtr sell_order = order_book.popBestSell();
            if (!sell_order) {
                order_book.addBuyOrder(std::move(buy_order));
                break;
            }

            // Execute trade
            PriceType trade_price = sell_order->price;
            int trade_quantity = std::min(buy_order->quantity, sell_order->quantity);

            TradeType trade(buy_order->id, sell_order->id, buy_order->symbol,
                          trade_price, trade_quantity);
            matched_trades.push_back(trade);

            // Update quantities
            buy_order->quantity -= trade_quantity;
            sell_order->quantity -= trade_quantity;

            // Re-add partially filled sell order
            if (sell_order->quantity > 0) {
                order_book.addSellOrder(std::move(sell_order));
            }
        }
    }

    void matchSellOrder(OrderPtr sell_order, std::vector<TradeType>& matched_trades) {
        while (sell_order && sell_order->quantity > 0) {
            // Check if we can match
            if (order_book.getBuyOrderCount() == 0) {
                // No buy orders, add sell order to book
                order_book.addSellOrder(std::move(sell_order));
                break;
            }

            PriceType best_bid = order_book.getBestBid();
            
            // Check if price matches
            if (sell_order->price > best_bid) {
                // Price doesn't match, add to book
                order_book.addSellOrder(std::move(sell_order));
                break;
            }

            // Match with best buy order
            OrderPtr buy_order = order_book.popBestBuy();
            if (!buy_order) {
                order_book.addSellOrder(std::move(sell_order));
                break;
            }

            // Execute trade
            PriceType trade_price = sell_order->price;
            int trade_quantity = std::min(buy_order->quantity, sell_order->quantity);

            TradeType trade(buy_order->id, sell_order->id, buy_order->symbol,
                          trade_price, trade_quantity);
            matched_trades.push_back(trade);

            // Update quantities
            buy_order->quantity -= trade_quantity;
            sell_order->quantity -= trade_quantity;

            // Re-add partially filled buy order
            if (buy_order->quantity > 0) {
                order_book.addBuyOrder(std::move(buy_order));
            }
        }
    }
};
