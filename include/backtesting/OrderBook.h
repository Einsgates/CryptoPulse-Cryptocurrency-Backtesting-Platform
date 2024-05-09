#pragma once

#include "../data/exchange.h"
#include "./order.h"

#include <map>
#include <queue>
#include <utility>

using namespace std;

/**
 * Class for orderbook.
 */
class OrderBook {
    public:
        /**
         * Constructor for OrderBook class.
         */
        OrderBook(std::shared_ptr<Exchange> exchange_,  MarketType market_type_, std::shared_ptr<Security> security_): exchange(exchange_), market_type(market_type_), security(security_) {}

        /**
         * Destructor for OrderBook class.
         */
        ~OrderBook() {}

        /**
         * Handles when data parser reads trade update.
         * @param price price the trade occurred. 
         * @param qty quantity of trade.
         * @return vector of pointer to our order, price, and size if any of our order got filled.
         */
        vector<tuple<std::shared_ptr<Order>, double, double>> tradeOccurred(double price, double qty) {
            if (price <= 0) {
                throw invalid_argument("Price should be positive");
                return vector<tuple<std::shared_ptr<Order>, double, double>>();
            }
            if (qty <= 0) {
                throw invalid_argument("Quantity should be positive");
                return vector<tuple<std::shared_ptr<Order>, double, double>>();
            }

            vector<tuple<std::shared_ptr<Order>, double, double>> fills;

            if (book[price].second.empty()) {
                return fills;
            }

            queue<pair<double, std::shared_ptr<Order>>>& level_queue = book[price].second;

            double quantity_to_fill = qty;
            while (quantity_to_fill != 0 && !level_queue.empty()) {
                double subtracting = min(quantity_to_fill, level_queue.front().first);
                level_queue.front().first -= subtracting;
                quantity_to_fill -= subtracting;

                if (level_queue.front().second != nullptr && level_queue.front().second->isLiveOrder()) {
                    fills.emplace_back(make_tuple(level_queue.front().second, price, subtracting));
                }

                if (level_queue.front().first == 0.0) {
                    level_queue.pop();
                }
            }

            return fills;
        }

        /**
         * Add order to the book.
         * @param price level price level of the new order
         * @param order_side order side of the new order
         * @param my_order boolean whether the new order is my order
         */
        pair<std::shared_ptr<Order>, vector<pair<double, double>>> addOrder(double price_level, int order_side, double order_size, std::shared_ptr<Order> order_ptr) {
            if (price_level <= 0) {
                throw invalid_argument("Price level should be positive");
                return  pair<std::shared_ptr<Order>, vector<pair<double, double>>>();
            }

            addLevel(price_level, order_side);  // Handling case where price level does not exist

            if (book[price_level].first != order_side) {
                if(order_ptr != nullptr && order_ptr->isLiveOrder()) {
                    return fillMarketOrder(order_ptr);
                }
            } else {
                if (book[price_level].second.empty()) {
                    book[price_level].second.push(make_pair(order_size, order_ptr));
                } else if (book[price_level].second.back().second == order_ptr) {
                    book[price_level].second.back().first += order_size;
                } else {
                    book[price_level].second.push(make_pair(order_size, order_ptr));
                }
            }

            return  pair<std::shared_ptr<Order>, vector<pair<double, double>>>();
        }

        /**
         * Fills limit/stop limit order instantly.
         * @param order_ptr pointer to order.
         * @param qty quantity that could get filled instantly
        */
        pair<std::shared_ptr<Order>, vector<pair<double, double>>> instantFillLimit(std::shared_ptr<Order> order_ptr, double qty) {
            if (!order_ptr->isLiveOrder()) {
                return pair<std::shared_ptr<Order>, vector<pair<double, double>>>();
            }

            double quantity_to_fill = qty;
            int side = order_ptr->getSide();

            double best_price = (side == 1) ? getBestAsk() : getBestBid();
            double not_our_order_total_size = getLevelNotOurOrderTotalSize(best_price);

            std::vector<std::pair<double, double>> fills;

            if (quantity_to_fill <= not_our_order_total_size) {
                // Can fill entirely at the best price level
                reduceOrder(best_price, quantity_to_fill);
                fills.emplace_back(best_price, quantity_to_fill);
            } else {
                // Partial fill at the best price level
                reduceOrder(best_price, not_our_order_total_size);
                if (not_our_order_total_size != 0) {
                    fills.emplace_back(best_price, not_our_order_total_size);
                }
                quantity_to_fill -= not_our_order_total_size;

                double next_level = side == 1 ? getNextSellSideLevel(best_price) : getNextBuySideLevel(best_price);

                while (next_level != -1 && quantity_to_fill > 0) {
                    not_our_order_total_size = getLevelNotOurOrderTotalSize(next_level);
                    double subtracting = min(quantity_to_fill, not_our_order_total_size);
                    reduceOrder(next_level, subtracting);
                    if (not_our_order_total_size != 0) {
                        fills.emplace_back(next_level, subtracting);
                    }
                    quantity_to_fill -= subtracting;

                    next_level = side == 1 ? getNextSellSideLevel(next_level) : getNextBuySideLevel(next_level);
                }

                fills.back().second += quantity_to_fill;
            }

            return {order_ptr, fills};
        }

        /**
         * Fills market/stop order.
         * @param order_ptr pointer to order.
        */
        pair<std::shared_ptr<Order>, vector<pair<double, double>>> fillMarketOrder(std::shared_ptr<Order> order_ptr) {
            if (!order_ptr->isLiveOrder()) {
                return pair<std::shared_ptr<Order>, vector<pair<double, double>>>();
            }

            double quantity_to_fill = order_ptr->getLeverageAdjustedBaseCurrencySize();
            int side = order_ptr->getSide();

            double best_price = (side == 1) ? getBestAsk() : getBestBid();
            double not_our_order_total_size = getLevelNotOurOrderTotalSize(best_price);

            std::vector<std::pair<double, double>> fills;

            if (quantity_to_fill <= not_our_order_total_size) {
                // Can fill entirely at the best price level
                reduceOrder(best_price, quantity_to_fill);
                fills.emplace_back(best_price, quantity_to_fill);
            } else {
                // Partial fill at the best price level
                reduceOrder(best_price, not_our_order_total_size);
                if (not_our_order_total_size != 0) {
                    fills.emplace_back(best_price, not_our_order_total_size);
                }
                quantity_to_fill -= not_our_order_total_size;
                double next_level = side == 1 ? getNextSellSideLevel(best_price) : getNextBuySideLevel(best_price);

                while (next_level != -1 && quantity_to_fill > 0) {
                    not_our_order_total_size = getLevelNotOurOrderTotalSize(next_level);
                    double subtracting = min(quantity_to_fill, not_our_order_total_size);
                    reduceOrder(next_level, subtracting);
                    if (not_our_order_total_size != 0) {
                        fills.emplace_back(next_level, subtracting);
                    }
                    quantity_to_fill -= subtracting;

                    next_level = side == 1 ? getNextSellSideLevel(next_level) : getNextBuySideLevel(next_level);
                }

                fills.back().second += quantity_to_fill;
            }

            return {order_ptr, fills};
        }

        /**
         * Handles updated buy side doing the following:
         *  1. Modify the provided price level; fill our orders if it was sell side and our order(s) was present
         *  2. Modify the price levels lower than the provided level; fill our orders if applicable
         * 
         * @param price_level price level to check.
         * @param order_size updated order size.
         * @return vector of pointer to our order, filled price and size if any of our resting orders got filled.
        */
        vector<tuple<std::shared_ptr<Order>, double, double>> buySideUpdated(double price_level, double order_size) {
            vector<tuple<std::shared_ptr<Order>, double, double>> filled_orders;
            addLevel(price_level, 1);  // Handling case where price level does not exist

            // First update the price level
            if (book[price_level].first == 1) {
                double not_our_order_size = getLevelNotOurOrderTotalSize(price_level);
                double our_order_size = getLevelOurOrderTotalSize(price_level);

                if (order_size > not_our_order_size + our_order_size) {
                    // Add order if updated order size is greater than original
                    addOrder(price_level, 1, order_size - getLevelTotalSize(price_level), nullptr);
                } else if (order_size < not_our_order_size + our_order_size) {
                    // Reduce order if updated order size is lesser than original
                    reduceOrder(price_level, min(not_our_order_size, not_our_order_size + our_order_size - order_size));
                }
            } else {
                if (getLevelOurOrderTotalSize(price_level) != 0) {
                    // Fill our orders
                    for (std::shared_ptr<Order> it : getOurOrderPtr(price_level)) {
                        if (it->isLiveOrder()) {
                            filled_orders.push_back(make_tuple(it, price_level, it->getLeverageAdjustedBaseCurrencySize()));
                        }
                    }
                }

                book[price_level] = std::make_pair(1, std::queue<pair<double, std::shared_ptr<Order>>>());
                addOrder(price_level, 1, order_size, nullptr);
            }

            // Any price level below price_level should also be buy side
            for (const auto& entry : book) {
                double key = entry.first;
                int value_first = entry.second.first;

                if (key < price_level && value_first == -1) {
                    if (getLevelOurOrderTotalSize(key) != 0) {
                        for (std::shared_ptr<Order> it : getOurOrderPtr(key)) {
                            if (it->isLiveOrder()) {
                                filled_orders.push_back(make_tuple(it, key, it->getLeverageAdjustedBaseCurrencySize()));
                            }
                        }
                    }
                    
                    book[key] = std::make_pair(1, std::queue<pair<double, std::shared_ptr<Order>>>());
                }
            }

            return filled_orders;
        }

        /**
         * Handles updated sell side doing the following:
         *  1. Modify the provided price level; fill our orders if it was sell buy and our order(s) was present
         *  2. Modify the price levels lower than the provided level; fill our orders if applicable
         * 
         * @param price_level price level to check.
         * @param order_size updated order size.
         * @return vector of pointer to our order, filled price and size if any of our resting orders got filled.
        */
        vector<tuple<std::shared_ptr<Order>, double, double>> sellSideUpdated(double price_level, double order_size) {
            vector<tuple<std::shared_ptr<Order>, double, double>> filled_orders;
            addLevel(price_level, -1);  // Handling case where price level does not exist

            // First update the price level
            if (book[price_level].first == -1) {
                double not_our_order_size = getLevelNotOurOrderTotalSize(price_level);
                double our_order_size = getLevelOurOrderTotalSize(price_level);

                if (order_size > not_our_order_size + our_order_size) {
                    // Add order if updated order size is greater than original
                    addOrder(price_level, -1, order_size - not_our_order_size - our_order_size, nullptr);
                } else if (order_size < not_our_order_size + our_order_size) {
                    // Reduce order if updated order size is lesser than original
                    reduceOrder(price_level, min(not_our_order_size, not_our_order_size + our_order_size - order_size));
                }
            } else {
                if (getLevelOurOrderTotalSize(price_level) != 0) {
                    // Fill our orders
                    for (std::shared_ptr<Order> it : getOurOrderPtr(price_level)) {
                        if (it->isLiveOrder()) {
                            filled_orders.push_back(make_tuple(it, price_level, it->getLeverageAdjustedBaseCurrencySize()));
                        }
                    }
                }

                book[price_level] = std::make_pair(-1, std::queue<pair<double, std::shared_ptr<Order>>>());
                addOrder(price_level, -1, order_size, nullptr);
            }

            // Any price level below price_level should also be buy side
            for (const auto& entry : book) {
                double key = entry.first;
                int value_first = entry.second.first;

                if (key > price_level && value_first == 1) {
                    if (getLevelOurOrderTotalSize(key) != 0) {
                        for (std::shared_ptr<Order> it : getOurOrderPtr(key)) {
                            if (it->isLiveOrder()) {
                                filled_orders.push_back(make_tuple(it, key, it->getLeverageAdjustedBaseCurrencySize()));
                            }
                        }
                    }
                    
                    book[key] = std::make_pair(-1, std::queue<pair<double, std::shared_ptr<Order>>>());
                }
            }

            return filled_orders;
        }

        /**
         * Getter for order side at price level.
         * @param price_level price level to check.
         * @return 1 if buy side, -1 if sell side, 0 if between best bid and ask.
         */
        int getOrderSide(double price_level) const {
            if (price_level <= 0) {
                throw invalid_argument("Price level should be positive");
                return 0;
            }

            double best_bid = getBestBid();
            double best_ask = getBestAsk();

            if (best_bid == -1.0 || best_ask == -1.0) {
                throw runtime_error("Orderbook has only one side");
                return 0;
            }

            if (price_level >= best_ask) {
                return -1; 
            } else if (price_level <= best_bid) {
                return 1;
            } else {
                return 0;
            }
        }

        /**
         * Getter for the best bid price.
         * @return best (highest) bid price.
         */
        double getBestBid() const {
            double best_bid = -1.0;

            for (const auto& entry : book) {
                const auto& key = entry.first;
                const auto& value = entry.second;

                if (value.first == 1 && key > best_bid) {
                    best_bid = key;
                }
            }

            return best_bid;
        }

        /**
         * Getter for the best ask price.
         * @return best (lowest) ask price.
         */
        double getBestAsk() const {
            double best_ask = std::numeric_limits<double>::max();

            for (const auto& entry : book) {
                const auto& key = entry.first;
                const auto& value = entry.second;

                if (value.first == -1 && key < best_ask) {
                    best_ask = key;
                }
            }
            return (best_ask == std::numeric_limits<double>::max()) ? -1.0: best_ask;
        }

        /**
         * Getter for total order size which is not our order at price level.
         * @param price_level price level to check.
         */
        double getLevelTotalSize(double price_level) const {
            if (price_level <= 0) {
                throw invalid_argument("Price level should be positive");
                return 0;
            }

            double total_size = 0.0;

            if (book.find(price_level) != book.end()) {
                const queue<pair<double, std::shared_ptr<Order>>>& order_queue = book.at(price_level).second;
                queue<pair<double, std::shared_ptr<Order>>> temp_queue = order_queue;

                while (!temp_queue.empty()) {
                    total_size += temp_queue.front().first;
                    temp_queue.pop();
                }
            } 

            return total_size;
        }

        double getLimitInstantFillQuantity(double price, int order_side) {
            if (order_side != 1 && order_side != -1) {
                throw invalid_argument("Order side should be 1 or -1");
                return -1.0;
            }

            double best_price = order_side == 1 ? getBestAsk() : getBestBid();
            double instant_fill_quantity = 0;

            if (order_side == 1) {
                while (best_price <= price && best_price != -1) {
                    instant_fill_quantity += getLevelNotOurOrderTotalSize(best_price);
                    best_price = getNextBuySideLevel(best_price);
                }
            } else {
                while (best_price >= price && best_price != -1) {
                    instant_fill_quantity += getLevelNotOurOrderTotalSize(best_price);
                    best_price = getNextSellSideLevel(best_price);
                }
            }

            return instant_fill_quantity;
        }

        /**
         * Getter for pointer to exchange.
         */
        std::shared_ptr<Exchange> getExchange() {return exchange;}

        /**
         * Getter for pointer to security.
         */
        std::shared_ptr<Security> getSecurity() {return security;}

        /**
         * Getter for market type.
         */
        MarketType getMarketType() {return market_type;}

    private:
        /**
         * Add level to the order book.
         * @param price_level price level to add
         */
        void addLevel(double price_level, int order_side) {
            if (book.find(price_level) != book.end()) {
                return;     // Return if price level already exists.
            }

            book[price_level] = std::make_pair(order_side, std::queue<pair<double, std::shared_ptr<Order>>>());
        }

        /**
         * Helper function to find the price level which is less than current_level
        */
        double getNextBuySideLevel(double current_level) {
            auto it = book.lower_bound(current_level);

            if (it != book.begin()) {
                --it;
                return it->first;
            } else {
                // No lower level found, return -1
                return -1;
            }
        }

        /**
         * Helper function to find the price level which is greater than current_level
         */
        double getNextSellSideLevel(double current_level) {
            auto it = book.upper_bound(current_level);

            if (it != book.end()) {
                return it->first; 
            } else {
                // No higher level found, return -1
                return -1;
            }
        }

        /**
         * Helper function to reduce the total order size of a given level.
         */
        void reduceOrder(double price_level, double num_to_reduce) {
            if (book.find(price_level) != book.end()) {
                auto& order_queue = book.at(price_level).second;

                // Convert the queue to a deque
                std::deque<std::pair<double, std::shared_ptr<Order>>> order_deque;
                while (!order_queue.empty()) {
                    order_deque.push_back(order_queue.front());
                    order_queue.pop();
                }

                // Iterate through the deque from back to front
                for (auto it = order_deque.rbegin(); it != order_deque.rend() && num_to_reduce > 0; ++it) {
                    auto& order_pair = *it;
                    if (order_pair.second == nullptr) {
                        // Subtract min(double in queue, num_to_reduce)
                        double reduction = std::min(order_pair.first, num_to_reduce);
                        order_pair.first -= reduction;
                        num_to_reduce -= reduction;

                        if (order_pair.first == 0) {
                            // Remove entry if quantity becomes 0
                            order_deque.pop_back();
                        }
                    }
                }

                // Store the updated deque back into the queue
                for (const auto& order_pair : order_deque) {
                    order_queue.push(order_pair);
                }
            }
        }

        /**
         * Getter for total order size which is not our order at price level.
         * @param price_level price level to check.
         */
        double getLevelNotOurOrderTotalSize(double price_level) const {
            if (price_level <= 0) {
                throw invalid_argument("Price level should be positive");
                return 0;
            }

            double total_size = 0.0;

            if (book.find(price_level) != book.end()) {
                const queue<pair<double, std::shared_ptr<Order>>>& order_queue = book.at(price_level).second;
                queue<pair<double, std::shared_ptr<Order>>> temp_queue = order_queue;

                while (!temp_queue.empty()) {
                    if (temp_queue.front().second == nullptr) {
                        total_size += temp_queue.front().first;
                    }
                    temp_queue.pop();
                }
            } 

            return total_size;
        }

        /**
         * Getter for total order size which is our order at price level.
         * @param price_level price level to check.
         */
        double getLevelOurOrderTotalSize(double price_level) const {
            if (price_level <= 0) {
                throw invalid_argument("Price level should be positive");
                return 0;
            }

            double total_size = 0.0;

            if (book.find(price_level) != book.end()) {
                const queue<pair<double, std::shared_ptr<Order>>>& order_queue = book.at(price_level).second;
                queue<pair<double, std::shared_ptr<Order>>> temp_queue = order_queue;

                while (!temp_queue.empty()) {
                    if (temp_queue.front().second != nullptr) {
                        total_size += temp_queue.front().first;
                    }
                    temp_queue.pop();
                }
            } 

            return total_size;
        }

        /**
         * Helper function that counts number of orders that is not ours in the order book.
         * @param price_level price level to check.
         * @return number of orders that is not ours.
         */
        int getNumNotOurOrder(double price_level) {
            int num_not_our_order = 0;

            if (book.find(price_level) != book.end()) {
                const auto& order_queue = book.at(price_level).second;
                std::queue<std::pair<double, std::shared_ptr<Order>>> temp_queue = order_queue;

                while (!temp_queue.empty()) {
                    const auto& order_pair = temp_queue.front();
                    if (order_pair.second == nullptr) {
                        // If the second element (shared_ptr<Order>) is nullptr
                        num_not_our_order++;
                    }
                    temp_queue.pop();
                }
            }  else {
                throw invalid_argument("Price level not found");
            }

            return num_not_our_order;
        }

        /**
         * Helper function that returns vector of shared pointer to our orders.
         * @param price_level price level to check.
         */
        vector<std::shared_ptr<Order>> getOurOrderPtr(double price_level) {
            std::vector<std::shared_ptr<Order>> result;

            if (book.find(price_level) != book.end()) {
                const auto& order_queue = book.at(price_level).second;

                // Iterate over the queue
                std::queue<std::pair<double, std::shared_ptr<Order>>> temp_queue = order_queue;
                while (!temp_queue.empty()) {
                    const auto& order_pair = temp_queue.front();
                    if (order_pair.second != nullptr) {
                        // Add non-null shared pointer to the result vector
                        result.push_back(order_pair.second);
                    }
                    temp_queue.pop();
                }
            }

            return result;
        }

        std::shared_ptr<Exchange> exchange;     /*< Exchange */
        std::shared_ptr<Security> security;     /*< Security */
        MarketType market_type;                 /*< Market type (Spot or Futures) */
        map<double, pair<int, queue<pair<double, std::shared_ptr<Order>>>>> book;      /*< Orderbook; key is price level; value is pair of order side (1 or -1) and queue of orders */
};