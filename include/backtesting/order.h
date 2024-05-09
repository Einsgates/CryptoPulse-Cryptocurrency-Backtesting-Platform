#pragma once

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../data/util.h"
#include "../data/exchange.h"
#include "../data/security.h"
#include "../data/timetype.h"

using namespace std;

/**
 * Abstract class for orders.
 * @todo Make sure the exchange's market type support certain order type
 * @todo Rate limit per session (later)
 * @todo Throw exception, catch and log the error
 */
class Order {
    public:
        /**
         * Constructor to create an order.
         */
        Order(std::shared_ptr<Security> security_, MarketType market_type_, std::shared_ptr<TimeType> timestamp_, string type_, int side_, double base_currency_size_, double quote_currency_size_, unsigned leverage_, MarginType margintype_ ,double price_, std::shared_ptr<Exchange> exchange_): 
                security(security_), market_type(market_type_), timestamp(timestamp_), type(type_), side(side_), base_currency_size(base_currency_size_), quote_currency_size(quote_currency_size_), leverage(leverage_), margin_type(margintype_), price(price_), exchange(exchange_) {
                    id = ++next_id; // Assign a unique order id
                    
                    // Sanity checking zero and negative inputs
                    if (side != 1 && side != -1) {
                        throw invalid_argument("Order side must be 1 (buy) or -1 (sell)");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (quote_currency_size == 0.0 && base_currency_size == 0.0) {
                        throw invalid_argument("Order size must be non-zero");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (quote_currency_size_ < 0.0) {
                        throw invalid_argument("Quote currency order size must be non-negative");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (base_currency_size_ < 0.0) {
                        throw invalid_argument("Base currency order size must be non-negative");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (price_ <= 0.0) {
                        throw invalid_argument("Order price must be positive");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (base_currency_size != 0 && quote_currency_size != 0) {
                        throw invalid_argument("Order size must only provided in base currency or quote currency, not both");
                        state = OrderState::Rejected;
                        return;
                    }

                    // Sanity checking price
                    if (countDigitsAfterDecimal(price) > countDigitsAfterDecimal(exchange->getTradingRules(market_type, *security)[0])) {
                        throw invalid_argument("Price must obey the minimum tick size");
                        state = OrderState::Rejected;
                        return;
                    }


                    // Sanity checking leverage
                    if (leverage < 1) {
                        throw invalid_argument("Leverage must be greater than or equal to 1");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (leverage == 1 && margin_type != MarginType::NoMargin) {
                        throw invalid_argument("If leverage is 1x, margin type must be 'NoMargin'");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (leverage != 1 && margin_type == MarginType::NoMargin) {
                        throw invalid_argument("If leverage is not 1x, margin type must not be 'NoMargin'");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (margin_type == MarginType::Isolated && leverage_ > exchange->getTradingRules(market_type, *security)[10]) {
                        throw invalid_argument("Leverage must not exceed the maximum isolated leverage limit");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (margin_type == MarginType::Cross && leverage_ > exchange->getTradingRules(market_type, *security)[11]) {
                        throw invalid_argument("Leverage must not exceed the maximum cross leverage limit");
                        state = OrderState::Rejected;
                        return;
                    }

                    // Sanity checking minimum order size
                    if (base_currency_size == 0) {
                        double base_currency_min_size = exchange->getTradingRules(market_type, *security)[1];
                        base_currency_size = int((quote_currency_size / price)/base_currency_min_size) * base_currency_min_size;
                    } else if (quote_currency_size == 0) {
                        quote_currency_size = base_currency_size * price;
                    }

                    leverage_adjusted_base_currency_size = leverage * base_currency_size;


                    if (leverage_adjusted_base_currency_size < exchange->getTradingRules(market_type, *security)[1]) {
                        throw invalid_argument("Order value must be greater than the minimum order value");
                        state = OrderState::Rejected;
                        return;
                    }
                    if (quote_currency_size * leverage < exchange->getTradingRules(market_type, *security)[2]) {
                        throw invalid_argument("Order value must be greater than the minimum order value");
                        state = OrderState::Rejected;
                        return;
                    }

                    // Sanity checking maximum trade size
                    if ((type == "LIMIT" || type == "STOPLIMIT") && (exchange->getTradingRules(market_type, *security)[3] != -1 && leverage_adjusted_base_currency_size > exchange->getTradingRules(market_type, *security)[3])) {
                        throw invalid_argument("Order value must not exceed the maximum limit order value");
                        state = OrderState::Rejected;
                        return;
                    }
                    if ((type == "LIMIT" || type == "STOPLIMIT") && (exchange->getTradingRules(market_type, *security)[4] != -1 && leverage * quote_currency_size > exchange->getTradingRules(market_type, *security)[4])) {
                        throw invalid_argument("Order value must not exceed the maximum limit order value");
                        state = OrderState::Rejected;
                        return;
                    }
                    if ((type == "MARKET" || type == "STOP") && (exchange->getTradingRules(market_type, *security)[5] != -1 && leverage_adjusted_base_currency_size > exchange->getTradingRules(market_type, *security)[5])) {
                        throw invalid_argument("Order value must not exceed the maximum market order value");
                        state = OrderState::Rejected;
                        return;
                    }
                    if ((type == "LIMIT" || type == "STOPLIMIT") && (exchange->getTradingRules(market_type, *security)[6] != -1 && leverage * quote_currency_size > exchange->getTradingRules(market_type, *security)[6])) {
                        throw invalid_argument("Order value must not exceed the maximum market order value");
                        state = OrderState::Rejected;
                        return;
                    }
                }

        /**
         * Destructor
         */
        ~Order() = default;

        /**
         * Checks if the order could be filled for limit and stop limit class
         * @param best_bid The best bid price of the given security
         * @param best_ask The best ask price of the given security
         * @return Boolean whether the order could be filled
         */
        virtual bool checkFillability(double best_bid, double best_ask) = 0;

        /**
         * Checks if the order could be triggered for stop and stop limit class
         */
        virtual void checkTriggered(double current_price) = 0;

        /**
         * Fills the order and change the order status accordingly
         * @param qty Base currency quantity to fill.
        */
        void fillOrder(double qty, double filled_price) {
            // Sanity checks
            if (qty < 0) {
                throw invalid_argument("Filled quantity should be non-negative");
                return;
            }
            if (qty == 0) {
                throw invalid_argument("Filled quantity should be non-zero");
                return;
            }
            if (qty > leverage_adjusted_base_currency_size) {
                throw invalid_argument("Filled quantity should not be greater than the order size");
            }

            if (qty == leverage_adjusted_base_currency_size) {
                average_fill_price = (average_fill_price * filled_size + filled_price * qty)/(filled_size+qty);
                base_currency_size = 0;
                quote_currency_size = 0;
                leverage_adjusted_base_currency_size = 0;
                state = OrderState::Filled;
            } else {
                average_fill_price = (average_fill_price * filled_size + filled_price * qty)/(filled_size+qty);
                filled_size += qty;
                leverage_adjusted_base_currency_size -= qty;
                base_currency_size /= leverage;
                quote_currency_size = base_currency_size * price;

                if (base_currency_size > 0) {
                    state = OrderState::PartiallyFilled;
                } else {
                    state = OrderState::Filled;
                }
            }
        }


        /**
         * Checks if the order is live or dead
         */
        bool isLiveOrder() const {return state == OrderState::Working || state == OrderState::PartiallyFilled;} 

        /**
         * Cancels the order if it is live.
         */
        void cancelOrder() {
            if (!isLiveOrder() && state != OrderState::SentToExchange) {return;}

            state = OrderState::Cancelled;
        }

        /**
         * Check if an exchange received the order and if so, change order status to working
         * @param current_timestamp Current timestamp in format 'yyyy-MM-dd HH:mm:ss.fffffffff'
         */
        void checkOrderReceived(TimeType& current_timestamp) {
            long long time_difference = current_timestamp.toNanosecondsSinceEpoch() - timestamp->toNanosecondsSinceEpoch();

            if (time_difference >= exchange->getSendingLatency()) {
                state = OrderState::Working;
            }   
        }

        /**
         * Reject order.
         */
        void rejectOrder() {state = OrderState::Rejected;}

        /**
         * Getter for unique id.
         */
        uint32_t getID() const {return id;}

        /**
         * Getter for security.
         */
        std::shared_ptr<Security> getSecurity() {return security;}

        /**
         * Getter for market type.
         */
        MarketType& getMarketType() {return market_type;}

        /**
         * Getter for timestamp.
         */
        TimeType& getTimestamp() const {return *timestamp;}

        /**
         * Getter for order type.
         */
        string getOrderType() const {return type;}

        /**
         * Getter for side.
         */
        int getSide() const {return side;}

        /**
         * Getter for base currency size.
         */
        double getBaseCurrencySize() const {return base_currency_size;}

        /**
         * Getter for quote currency size.
         */
        double getQuoteCurrencySize() const {return quote_currency_size;}

        /**
         * Getter for leverage
         */
        unsigned getLeverage() const {return leverage;}

        /**
         * Getter for leverage adjusted base currency size.
         */
        double getLeverageAdjustedBaseCurrencySize() const {return leverage_adjusted_base_currency_size;}

        /**
         * Getter for filled size.
         */
        double getFilledSize() const {return filled_size;}

        /**
         * Getter for price.
         */
        double getPrice() const {return price;}

        /**
         * Getter for orderState.
         */
        OrderState getOrderState() const {return state;}

        /**
         * Getter for exchange.
         */
        std::shared_ptr<Exchange> getExchange() {return exchange;}

        /**
         * Getter for margin type.
         */
        MarginType& getMarginType() {return margin_type;}

        /**
         * Finds the number of digits after the decimal point
         * @param value number to find the number of decimal digits
         * @return number of decimal digits
         */
        int countDigitsAfterDecimal(double value) {
            string valueStr = to_string(value);
            size_t decimalPos = valueStr.find('.');
            if (decimalPos != string::npos) {
                return valueStr.length() - decimalPos - 1;
            }
            return 0; // No decimal point found
        }

    protected:
        int id;     /*< Unique ID */
        MarketType market_type;   /*< Market type */
        std::shared_ptr<Security> security;        /*< Security */
        std::shared_ptr<TimeType> timestamp;       /*< UTC timestamp */
        string type;              /*< Order type */
        const int side;                 /*< -1 for SELL, 1 for BUY */
        double base_currency_size;      /*< Quantity of the order in base currency before leverage (For BTC/USDT, order size in BTC) */
        double quote_currency_size;     /*< Quantity of the order in quote currency before leverage (For BTC/USDT, order size in USDT) */
        unsigned leverage;              /*< Leverage of the order */
        MarginType margin_type;   /*< Margin type of the order */
        double leverage_adjusted_base_currency_size;    /*< Leverage adjusted base currency size */
        double filled_size = 0.0;       /*< Quantify filled of the order */
        double average_fill_price = 0.0;      /*< Average filled price of the order */
        double price;                   /*< Price for LIMIT and STOP LIMIT orders. Current price for MARKET and STOP orders */
        std::shared_ptr<Exchange> exchange;        /*< Target exchange for an order */
        OrderState state = OrderState::SentToExchange;         /*< State of the order */
        vector<int> child_trade_id;     /*< Vector of trade ids executed from this order */

    private:
        static int next_id; /*< Static member to track the next available ID */ 
};


/**
 * Limit order class
 */
class Limit : public Order {
public:
    /**
     * Constructor to create an order.
     */
    Limit(std::shared_ptr<Security> security_, MarketType market_type_ , std::shared_ptr<TimeType> timestamp_, int side_, double base_currency_size_, double quote_currency_size_, unsigned leverage_, MarginType margintype_, double price_, std::shared_ptr<Exchange> exchange_): 
            Order(security_, market_type_, timestamp_, "LIMIT", side_, base_currency_size_, quote_currency_size_, leverage_, margintype_, price_, exchange_) {}

    /**
     * Checks if the order could be filled for limit and stop limit class
     * @param best_bid The best bid price of the given security
     * @param best_ask The best ask price of the given security
     * @return Boolean whether the order could be filled
     */
    bool checkFillability(double best_bid, double best_ask) override {
        if (!isLiveOrder()) {return false;}
        if (side == 1 && price >= best_ask) {return true;}
        if (side == -1 && price <= best_bid) {return true;}

        return false;
    }

    void checkTriggered(double current_price) override {
        return;
    }

    /**
     * Modifies the order. 
     * @param modified_base_currency_size New size of the order in base currency
     * @param modified_quote_currency_size New size of the order in quote currency
     * @param modified_price New price of the order
     */
    void modifyOrder(double modified_base_currency_size, double modified_quote_currency_size, double modified_price) {
        // Sanity checks
        if (!isLiveOrder()) {return;}
        if (modified_price <= 0.0) {
            throw invalid_argument("Modified price must be positive");
            return;
        }
        if (modified_base_currency_size == 0.0 && modified_quote_currency_size == 0.0) {
            throw invalid_argument("Order size must be non-zero");
            return;
        }
        if (modified_base_currency_size < 0.0) {
            throw invalid_argument("Modified base currency size must not be negative");
            return;
        }
        if (modified_quote_currency_size < 0.0) {
            throw invalid_argument("Modified quote currency size must not be negative");
            return;
        }
        if (base_currency_size != 0 && quote_currency_size != 0) {
            throw invalid_argument("Order size must only provided in base currency or quote currency, not both");
            return;
        }
        if (countDigitsAfterDecimal(modified_price) > countDigitsAfterDecimal(exchange->getTradingRules(market_type, *security)[0])) {
            throw invalid_argument("Modified price must obey the minimum tick size");
            return;
        }

        if (modified_base_currency_size == 0) {
            quote_currency_size = modified_quote_currency_size;
            double base_currency_min_size = exchange->getTradingRules(market_type, *security)[1];
            base_currency_size = int((modified_quote_currency_size / modified_price)/base_currency_min_size) * base_currency_min_size;
        } else if (modified_quote_currency_size == 0) {
            base_currency_size = modified_base_currency_size;
            quote_currency_size = base_currency_size * modified_price;
        }

        leverage_adjusted_base_currency_size = leverage * base_currency_size;
    }
};


/**
 * Market order class
 */
class Market : public Order {
public:
    /**
     * Constructor to create an order.
     */
    Market(std::shared_ptr<Security> security_, MarketType market_type_, std::shared_ptr<TimeType> timestamp_, int side_, double base_currency_size_, double quote_currency_size_, unsigned leverage_, MarginType margintype_, double current_price_, std::shared_ptr<Exchange> exchange_): 
            Order(security_, market_type_, timestamp_, "MARKET", side_, base_currency_size_, quote_currency_size_, leverage_, margintype_, current_price_, exchange_) {}

    /**
     * Checks if the order could be filled for limit and stop limit class
     * @param best_bid The best bid price of the given security
     * @param best_ask The best ask price of the given security
     * @return Boolean whether the order could be filled
     */
    bool checkFillability(double best_bid, double best_ask) override {
        return isLiveOrder();
    }

    void checkTriggered(double current_price) override {
        return;
    }
};


/**
 * Stop order class
 */
class Stop : public Order {
public:
    /**
     * Constructor to create an order.
     */
    Stop(std::shared_ptr<Security> security_, MarketType market_type_, std::shared_ptr<TimeType> timestamp_, int side_, double base_currency_size_, double quote_currency_size_, unsigned leverage_, MarginType margintype_, double trigger_price_, std::shared_ptr<Exchange> exchange_): 
            Order(security_, market_type_, timestamp_, "STOP", side_, base_currency_size_, quote_currency_size_, leverage_, margintype_, trigger_price_, exchange_) {
                triggered = false, trigger_price = trigger_price_;
            }

    /**
     * Checks if the order could be filled for limit and stop limit class
     * @param best_bid The best bid price of the given security
     * @param best_ask The best ask price of the given security
     * @return Boolean whether the order could be filled
     */
    bool checkFillability(double best_bid, double best_ask) override {
        if (!isLiveOrder() || !triggered) {return false;}

        return true;
    }

    /**
     * Checks if the order could be triggered based on current price.
     * @param current_price Current trading price of a security.
     * @return Boolean whether the order is triggered
     */
    void checkTriggered(double current_price) override {
        if (current_price <= 0.0) {
            throw invalid_argument("Current price must be positive");
            return;
        }

        if ((side == 1 && current_price >= trigger_price) || (side == -1 && current_price <= trigger_price)) {
            triggered = true;
        }
    }

    /**
     * Modifies the order. Could be used not only when user changes the order, but also when the order is partially filled.
     * @param modified_base_currency_size New size of the order in base currency
     * @param modified_quote_currency_size New size of the order in quote currency
     * @param modified_trigger_price New trigger price of the order
     */
    void modifyOrder(double modified_base_currency_size, double modified_quote_currency_size, double modified_trigger_price) {
        // Sanity checks
        if (!isLiveOrder() || triggered) {return;}
        if (modified_base_currency_size == 0.0 && modified_quote_currency_size == 0.0) {
            throw invalid_argument("Order size must be non-zero");
            return;
        }
        if (modified_base_currency_size < 0.0) {
            throw invalid_argument("Modified base currency order size must not be negative");
            return;
        }
        if (modified_quote_currency_size < 0.0) {
            throw invalid_argument("Modified quote currency order size must not be negative");
            return;
        }
        if (base_currency_size != 0 && quote_currency_size != 0) {
            throw invalid_argument("Order size should only provided in base currency or quote currency, not both");
            return;
        }
        if (modified_trigger_price <= 0) {
            throw invalid_argument("Modified trigger price must be positive");
            return;
        }
        if (countDigitsAfterDecimal(modified_trigger_price) > countDigitsAfterDecimal(exchange->getTradingRules(market_type, *security)[0])) {
            throw invalid_argument("Modified trigger price must obey the minimum tick size");
            return;
        }
        
       if (modified_base_currency_size == 0) {
            quote_currency_size = modified_quote_currency_size;
            double base_currency_min_size = exchange->getTradingRules(market_type, *security)[1];
            base_currency_size = int((modified_quote_currency_size / modified_trigger_price)/base_currency_min_size) * base_currency_min_size;
        } else if (modified_quote_currency_size == 0) {
            base_currency_size = modified_base_currency_size;
            quote_currency_size = base_currency_size * modified_trigger_price;
        }

        leverage_adjusted_base_currency_size = leverage * base_currency_size;
        trigger_price = modified_trigger_price;
    }

    /**
     * Getter for triggered.
     */
    bool isTriggered() const {return triggered;}

    /**
     * Getter for trigger price.
     */
    double getTriggerPrice() const {return trigger_price;}

private:
    bool triggered;         /*< Whether the trigger price has been hit */
    double trigger_price;   /*< Trigger price for the stop order */
};



/**
 * Stop Limit order class
 */
class StopLimit : public Order {
public:
    /**
     * Constructor to create an order.
     */
    StopLimit(std::shared_ptr<Security> security_, MarketType market_type_, std::shared_ptr<TimeType> timestamp_, int side_, double base_currency_size_, double quote_currency_size_, unsigned leverage_, MarginType margintype_, double price_, double trigger_price_, std::shared_ptr<Exchange> exchange_): 
            Order(security_, market_type_, timestamp_, "STOPLIMIT", side_, base_currency_size_, quote_currency_size_, leverage_, margintype_, price_, exchange_) {
                triggered = false, trigger_price = trigger_price_;
            }

    /**
     * Checks if the order could be filled for limit and stop limit class
     * @param best_bid The best bid price of the given security
     * @param best_ask The best ask price of the given security
     * @return Boolean whether the order could be filled
     */
    bool checkFillability(double best_bid, double best_ask) override {
        if (!isLiveOrder() || !triggered) {return false;}

        if (side == 1 && price >= best_ask) {return true;}
        if (side == -1 && price <= best_bid) {return true;}

        return false;
    }

    /**
     * Checks if the order could be triggered based on current price.
     * @param current_price Current trading price of a security.
     * @return Boolean whether the order is triggered
     */
    void checkTriggered(double current_price) override {
        if (current_price <= 0.0) {
            throw invalid_argument("Current price must be positive");
            return;
        }

        if ((side == 1 && current_price >= trigger_price) || (side == -1 && current_price <= trigger_price)) {
            triggered = true;
        }
    }

    /**
     * Modifies the order. Could be used not only when user changes the order, but also when the order is partially filled.
     * @param modified_base_currency_size New size of the order in base currency
     * @param modified_quote_currency_size New size of the order in quote currency
     * @param modified_price New price of the order
     * @param modified_trigger_price New trigger price of the order. If it has been triggered, should input the original trigger price.
     */
    void modifyOrder(double modified_base_currency_size, double modified_quote_currency_size, double modified_price, double modified_trigger_price) {
        // Sanity checks
        if (!isLiveOrder()) {return;}
        if (modified_trigger_price != trigger_price && triggered) {
            throw invalid_argument("Trigger price cannot be modified once triggered");
            return;
        }
        if (modified_trigger_price <= 0.0) {
            throw invalid_argument("Trigger price must be positive");
            return;
        }
        if (modified_base_currency_size == 0.0 && modified_quote_currency_size == 0.0) {
            throw invalid_argument("Order size must be non-zero");
            return;
        }
        if (modified_base_currency_size < 0.0) {
            throw invalid_argument("Modified base currency size must not be negative");
            return;
        }
        if (modified_quote_currency_size < 0.0) {
            throw invalid_argument("Modified quote currency size must not be negative");
            return;
        }
        if (base_currency_size != 0 && quote_currency_size != 0) {
            throw invalid_argument("Order size should only provided in base currency or quote currency, not both");
            return;
        }
        if (countDigitsAfterDecimal(modified_price) > countDigitsAfterDecimal(exchange->getTradingRules(market_type, *security)[0])) {
            throw invalid_argument("Modified price must obey the minimum tick size");
            return;
        }
        if (countDigitsAfterDecimal(modified_trigger_price) > countDigitsAfterDecimal(exchange->getTradingRules(market_type, *security)[0])) {
            throw invalid_argument("Modified trigger price must obey the minimum tick size");
            return;
        }

        if (modified_base_currency_size == 0) {
            quote_currency_size = modified_quote_currency_size;
            double base_currency_min_size = exchange->getTradingRules(market_type, *security)[1];
            base_currency_size = int((modified_quote_currency_size / modified_price)/base_currency_min_size) * base_currency_min_size;
        } else if (modified_quote_currency_size == 0) {
            base_currency_size = modified_base_currency_size;
            quote_currency_size = base_currency_size * modified_price;
        }

        leverage_adjusted_base_currency_size = leverage * base_currency_size;

        double new_base_currency_size = modified_base_currency_size;
        double new_quote_currency_size = modified_quote_currency_size;

        trigger_price = modified_trigger_price;
    }

    /**
     * Getter for triggered.
     */
    bool isTriggered() const {return triggered;}

    /**
     * Getter for trigger price.
     */
    double getTriggerPrice() const {return trigger_price;}

private:
    bool triggered;         /*< Whether the trigger price has been hit */
    double trigger_price;   /*< Trigger price for the stop order */
};

int Order::next_id = 0;