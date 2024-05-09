#pragma once


#include "./exchange.h"
#include "./security.h"
#include "./timetype.h"
#include "../backtesting/orderbook.h"

#include <string>

using namespace std;


/**
 * Parent class for all event messages
 */
class EventMsg {
public:
    EventMsg(std::shared_ptr<TimeType> timestamp_, std::shared_ptr<Exchange> exchange_, MarketType market_type_, std::shared_ptr<Security> security_, std::shared_ptr<OrderBook> orderbook_)
        : timestamp(timestamp_), exchange(exchange_), market_type(market_type_), security(security_), orderbook(orderbook_) {}

    std::shared_ptr<TimeType> timestamp;    /*< timestamp of an event */
    std::shared_ptr<Exchange> exchange;     /*< exchange an event occurred */
    MarketType market_type;                 /*< Market type (Futures or spot) */
    std::shared_ptr<Security> security;     /*< Instrument an event occurred */
    std::shared_ptr<OrderBook> orderbook;   /*< Orderbook */
};

/**
 * Derived class for trade messages
 */
class TradeEventMsg : public EventMsg {
public:
    TradeEventMsg(std::shared_ptr<TimeType> timestamp_, std::shared_ptr<Exchange> exchange_, MarketType market_type_, std::shared_ptr<Security> security_, std::shared_ptr<OrderBook> orderbook_, double price_, double size_): 
            EventMsg(timestamp_, exchange_, market_type_, security_, orderbook_), price(price_), size(size_) {}

    double price;  /*< price at which the trade occurred */
    double size;   /*< size of the trade in base currency */ 

    /**
     * Override << operator for Market class.
     */
    friend ostream& operator<<(ostream& os, TradeEventMsg& msg) {
        os << "========== Trade Event ==========" << endl;
        os << "Timestamp: " << msg.timestamp->toString() << endl;
        os << "Market: " << msg.market_type << endl;
        os << "Security: " << msg.security << endl;
        os << "Exchange: " << msg.exchange->getName() << endl;
        os << "Price: " << msg.price << endl;
        os << "Size: " << msg.size << msg.security->getBase() << endl;
        os << "=======================================" << endl;
        return os;
    }
};


/**
 * Derived class for quote update messages
 */ 
class QuoteEventMsg : public EventMsg {
public:
    QuoteEventMsg(std::shared_ptr<TimeType> timestamp_, std::shared_ptr<Exchange> exchange_, MarketType market_type_, std::shared_ptr<Security> security_, std::shared_ptr<OrderBook> orderbook_, double bid_price_, double bid_size_, double ask_price_, double ask_size_): 
        EventMsg(timestamp_, exchange_, market_type_, security_, orderbook_), bid_price(bid_price_), bid_size(bid_size_), ask_price(ask_price_), ask_size(ask_size_) {}

    double bid_price;  /*< best bid price */
    double bid_size;   /*< best bid size in base currency */
    double ask_price;  /*< best ask price */
    double ask_size;   /*< best ask size in base currency */

    /**
     * Override << operator for Market class.
     */
    friend ostream& operator<<(ostream& os, QuoteEventMsg& msg) {
        os << "========== Quote Update ==========" << endl;
        os << "Timestamp: " << msg.timestamp->toString() << endl;
        os << "Market: " << msg.market_type << endl;
        os << "Security: " << msg.security << endl;
        os << "Exchange: " << msg.exchange->getName() << endl;
        os << "Bid price: " << msg.bid_price << endl;
        os << "Bid size: " << msg.bid_size << msg.security->getBase() << endl;
        os << "Ask price: " << msg.ask_price << endl;
        os << "Ask size: " << msg.ask_size << msg.security->getBase() << endl;
        os << "=======================================" << endl;
        return os;
    }
};


/**
 * Derived class for depth messages
 */ 
class DepthEventMsg : public EventMsg {
public:
    DepthEventMsg(std::shared_ptr<TimeType> timestamp_, std::shared_ptr<Exchange> exchange_, MarketType market_type_, std::shared_ptr<Security> security_, std::shared_ptr<OrderBook> orderbook_, int side_, double price_, double size_): 
        EventMsg(timestamp_, exchange_, market_type_, security_, orderbook_), side(side_), price(price_), size(size_) {}

    int side;      /*< indicates bid (1) or ask (-1) side of the order book */
    double price;  /*< price for the entry in the order book */
    double size;   /*< base currency size associated with the book entry */

    /**
     * Override << operator for Market class.
     */
    friend ostream& operator<<(ostream& os, DepthEventMsg& msg) {
        os << "========== Quote Update ==========" << endl;
        os << "Timestamp: " << msg.timestamp->toString() << endl;
        os << "Market: " << msg.market_type << endl;
        os << "Security: " << msg.security << endl;
        os << "Exchange: " << msg.exchange->getName() << endl;
        os << "Side: " << msg.side << endl;
        os << "Price: " << msg.price << msg.security->getBase() << endl;
        os << "Size: " << msg.size << endl;
        os << "=======================================" << endl;
        return os;
    }
};
