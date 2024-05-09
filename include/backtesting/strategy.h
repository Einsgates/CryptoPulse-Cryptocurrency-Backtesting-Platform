#pragma once

#include "order.h"
#include "user.h"
#include "../data/eventmsg.h"
#include "../data/exchange.h"
#include "../data/security.h"

#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace std;


/**
 * Custom hash function for tuple<MarketType, Exchange, Security>
 */
struct MarketKeyHash {
    size_t operator()(const tuple<MarketType, Exchange, Security>& key) const {
        // Combine the hash values of the components
        size_t marketHash = static_cast<size_t>(get<0>(key));
        size_t exchangeHash = Exchange::Hash{}(get<1>(key));
        size_t securityHash = Security::Hash{}(get<2>(key));

        // Combine the hash values using XOR and shift
        return marketHash ^ (exchangeHash << 1) ^ (securityHash << 2);
    }
};


/**
 * Abstract class for strategy
 */
class Strategy {
    public:
        using MarketKey = tuple<MarketType, Exchange, Security>;
        using MarketMap = unordered_map<MarketKey, double, MarketKeyHash>;
        
        /**
         * Default constructor
         */
        Strategy() {}

        /**
         * Constructor
         */
        Strategy(const string& strategy_name_, User& user_): strategy_name(strategy_name_), user(user_) {}

        /**
         * Destructor
         */
        virtual ~Strategy() = default;

        /**
         * Clear/Reset members
         */
        virtual void Clear() = 0;

        /**
         * Triggers when a trade event message arrives
         * @param event_msg trade event message
         * @return vector of orders to submit. Return empty vector if no orders to submit.
         */
        virtual vector<std::shared_ptr<Order>> onTrade(TradeEventMsg& event_msg) = 0;

        /**
         * Triggers when a change in top quote (BBO) message arrives
         * @param event_msg BBO update event message
         * @return vector of orders to submit. Return empty vector if no orders to submit.
         */
        virtual vector<std::shared_ptr<Order>> onTopQuote(QuoteEventMsg& event_msg) = 0;

        /**
         * Triggers when a change in orderbook message arrives
         * @param event_msg order book update event message
         * @return vector of orders to submit. Return empty vector if no orders to submit.
         */
        virtual vector<std::shared_ptr<Order>> onDepth(DepthEventMsg& event_msg) = 0;

        /**
         * Getter for position
         * @param marketType market type (spot or futures)
         * @param exchange exchange
         * @param security security
         */
        double getPosition(MarketType marketType, const Exchange& exchange, const Security& security) {
            MarketKey key = make_tuple(marketType, exchange, security);
            auto it = position.find(key);

            if (it != position.end()) {
                return it->second;
            }

            position[key] = 0.0;
            return 0.0;
        }

        /**
         * Getter for the map itself.
         */
        MarketMap getPositionMap() const {return position;}

        /**
         * Update position
         * @param marketType market type (spot or futures)
         * @param exchange exchange
         * @param security security
         * @param position_change change in position
         */
        void updatePosition(MarketType marketType, const Exchange& exchange, const Security& security, double position_change) {
            MarketKey key = make_tuple(marketType, exchange, security);
            position[key] += position_change;
        }


    protected:
        const string strategy_name;
        User user;
        MarketMap position; 
};