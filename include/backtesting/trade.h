#pragma once

#include <string>
#include "./order.h"
#include "../data/exchange.h"
#include "../data/security.h"

using namespace std;


/**
 * Abstract class for trades.
 */
class Trade {
    public:
        /**
         * Constructor to create a trade.
         */
        Trade(std::shared_ptr<Order> parent_order_, std::shared_ptr<TimeType> timestamp_, int side_, double base_currency_size_, double price_, bool is_maker_): 
                parent_order(parent_order_), timestamp(timestamp_), side(side_), base_currency_size(base_currency_size_), price(price_), is_maker(is_maker_) {
                    id = ++next_id; // Assign a unique order id

                    fee = is_maker ? base_currency_size * price * parent_order->getExchange()->getMakerFee(parent_order->getMarketType()) / 100 : base_currency_size * price * parent_order->getExchange()->getTakerFee(parent_order->getMarketType()) / 100;
                }

        /**
         * Destructor
         */
        virtual ~Trade() {}

        /**
         * Getter for trade ID.
         */
        int getID() const { return id; }

        /**
         * Getter for parent order.
         */
        std::shared_ptr<Order> getParentOrder() const {return parent_order;}

        /**
         * Getter for security.
         */
        std::shared_ptr<Security> getSecurity() const {return parent_order->getSecurity();}

        /**
         * Getter for timestamp.
         */
        std::shared_ptr<TimeType> getTimestamp() const {return timestamp;}

        /**
         * Getter for side.
         */
        int getSide() {return side;}

        /**
         * Getter for size.
         */
        double getBaseCurrencySize() {return base_currency_size;}

        /**
         * Getter for price.
         */
        double getPrice() {return price;}

        /**
         * Getter for exchange.
         */
        std::shared_ptr<Exchange> getExchange() const {return parent_order->getExchange();}

        /**
         * Getter for boolean if the trade was making or taking
         */
        bool getIfMaker() const {return is_maker;}

        /**
         * Getter for trade fee
         */
        double getFee() const {return fee;}

    private:
        int id;                     /*< Unique id */
        static int next_id;         /*< Static member to track the next available ID */ 
        std::shared_ptr<Order> parent_order;    /*< Parent order */
        std::shared_ptr<TimeType> timestamp;    /*< UTC timestamp */
        int side;                   /*< -1 for SELL, 1 for BUY */
        double base_currency_size;  /*< Quantity of the order in base currency */
        double price;               /*< Price at which trade occurred */
        bool is_maker;              /*< Whether the trade was making or taking */
        double fee;                 /*< Execution fee associated with the order */
};

int Trade::next_id = 0;