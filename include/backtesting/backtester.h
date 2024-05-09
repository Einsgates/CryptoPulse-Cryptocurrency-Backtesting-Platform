#pragma once

#include "./order.h"
#include "./orderbook.h"
#include "./strategy.h"
#include "./trade.h"
#include "./user.h"
#include "../data/exchange.h"
#include "../data/security.h"
#include "../record/tradelog.h"
#include <boost/accumulators/accumulators.hpp>
#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


using namespace std;
using namespace boost;



/**
 * Class for backtesting
 */
class Backtester {
    public:
        using MarketKey = tuple<MarketType, Exchange, Security>;
        using MarketMap = unordered_map<MarketKey, std::shared_ptr<OrderBook>, MarketKeyHash>;

    /**
     * Constructor
     */
    Backtester(User& user_, Strategy* strategy_): user(user_), strategy(strategy_) {
        // Setup Orderbooks
        loadOrderBook();
    }

    /**
     * Destructor
     */
    ~Backtester() {}

    /**
     * Clears/Resets all members
    */
    void Clear(map<MarketType, double> initial_buying_power) {
        orderbooks.clear();
        loadOrderBook();
        orderlog.Clear();
        tradelog.Clear();
        last_traded_price.clear();
        user.Clear(initial_buying_power);
        strategy->Clear();
    }

    /**
     * Load Orderbook data.
     */
    void loadOrderBook() {
        for (auto&& exchanges : user.getExchanges()) {
            for (auto&& securities : exchanges->getListedSecurities(MarketType::Spot)) {
                orderbooks[make_tuple(MarketType::Spot, *exchanges, *securities)] = make_shared<OrderBook>(exchanges, MarketType::Spot, securities);
            }
            for (auto&& securities : exchanges->getListedSecurities(MarketType::Futures)) {
                orderbooks[make_tuple(MarketType::Futures, *exchanges, *securities)] = make_shared<OrderBook>(exchanges, MarketType::Futures, securities);
            }
        }
    }

    /**
     * Run backtest
     * 
     * Backtest steps:
     *  1. Open the input data path and read/parse line-by-line
     *  2. While reading line-by-line call strategy functions accordingly
     *  3. Also, update orderbooks and check trade fillabilities
     *  4. Record all trades
     * 
     * @param data_path file path for market data input
     */
    void runBacktest(const string& data_path) {
        vector<std::shared_ptr<Order>> current_orders;
        vector<TimeType*> timetype_vector;
        ifstream file(data_path);

        if (!file.is_open()) {
            throw invalid_argument("Error opening the file");
            return;
        }

        int n = 2;
        string line;
        getline(file, line);    // Skip first line
        while (getline(file, line)) {
            vector<string> tokens;
            boost::split(tokens, line, boost::is_any_of(","));

            // Finding Exchange
            std::shared_ptr<Exchange> exchange_ptr = user.findExchange(tokens[4]);

            if (exchange_ptr == nullptr) {
                throw runtime_error("Exchange " + tokens[4] + " is not found");
            }

            // Finding Security
            std::shared_ptr<Security> security_ptr = exchange_ptr->findSecurity(MarketType::Spot, tokens[3]);

            if (security_ptr == nullptr) {
                throw runtime_error("Security " + tokens[3] + " is not found");
            }

            // Calling strategy functions
            vector<std::shared_ptr<Order>> order_vector;
            MarketType mt = tokens[5] == "S" ? MarketType::Spot : MarketType::Futures;
            std::shared_ptr<TimeType> tt = std::make_shared<TimeType>(tokens[0]);
            std::shared_ptr<OrderBook> ob = getOrderbook(mt, *exchange_ptr, *security_ptr);


            if (tokens[2] == "T") {
                last_traded_price[make_pair(mt, security_ptr)] = stod(tokens[6]);
                vector<tuple<std::shared_ptr<Order>, double, double>> filled_orders = ob->tradeOccurred(last_traded_price[make_pair(mt, security_ptr)], stod(tokens[7]));

                for (auto it : filled_orders) {
                    std::get<0>(it)->fillOrder(std::get<2>(it), std::get<1>(it));

                    Trade* trade = new Trade(std::get<0>(it), tt, std::get<0>(it)->getSide(), std::get<2>(it), std::get<1>(it), true);
                    tradelog.addTrade(std::shared_ptr<Trade>(trade));

                    user.updateBalance(std::get<0>(it)->getMarketType(), -trade->getSide() * std::get<2>(it) * std::get<1>(it) - trade->getFee());
                    strategy->updatePosition(std::get<0>(it)->getMarketType(), *std::get<0>(it)->getExchange(), *std::get<0>(it)->getSecurity(), std::get<0>(it)->getSide()*std::get<2>(it));
                }


                TradeEventMsg msg(tt, exchange_ptr, mt, security_ptr, ob, stod(tokens[6]), stod(tokens[7]));
                order_vector = strategy->onTrade(msg);
            }

            else if (tokens[2] == "BID_UPDATE" || tokens[2] == "ASK_UPDATE") {
                vector<tuple<std::shared_ptr<Order>, double, double>> filled_orders = tokens[2] == "BID_UPDATE" ? ob->buySideUpdated(stod(tokens[8]), stod(tokens[9])) 
                        : ob->sellSideUpdated(stod(tokens[14]), stod(tokens[15]));

                for (auto it : filled_orders) {
                    std::get<0>(it)->fillOrder(std::get<2>(it), std::get<1>(it));

                    Trade* trade = new Trade(std::get<0>(it), tt, std::get<0>(it)->getSide(), std::get<2>(it), std::get<1>(it), true);
                    tradelog.addTrade(std::shared_ptr<Trade>(trade));

                    user.updateBalance(std::get<0>(it)->getMarketType(), -trade->getSide() * std::get<2>(it) * std::get<1>(it) - trade->getFee());
                    strategy->updatePosition(std::get<0>(it)->getMarketType(), *std::get<0>(it)->getExchange(), *std::get<0>(it)->getSecurity(), std::get<0>(it)->getSide()*std::get<2>(it));
                }

                QuoteEventMsg msg(tt, exchange_ptr, mt, security_ptr, ob, stod(tokens[8]), stod(tokens[9]), stod(tokens[14]), stod(tokens[15]));
                order_vector = strategy->onTopQuote(msg);
            }

            else if (tokens[2] == "BUY_SIDE_UPDATE" || tokens[2] == "SELL_SIDE_UPDATE") {
                vector<tuple<std::shared_ptr<Order>, double, double>> filled_orders = tokens[2] == "BUY_SIDE_UPDATE" ? ob->buySideUpdated(stod(tokens[6]), stod(tokens[7])) 
                        : ob->sellSideUpdated(stod(tokens[6]), stod(tokens[7]));

                for (auto it : filled_orders) {
                    std::get<0>(it)->fillOrder(std::get<2>(it), std::get<1>(it));

                    Trade* trade = new Trade(std::get<0>(it), tt, std::get<0>(it)->getSide(), std::get<2>(it), std::get<1>(it), true);
                    tradelog.addTrade(std::shared_ptr<Trade>(trade));

                    user.updateBalance(std::get<0>(it)->getMarketType(), -trade->getSide() * std::get<2>(it) * std::get<1>(it) - trade->getFee());
                    strategy->updatePosition(std::get<0>(it)->getMarketType(), *std::get<0>(it)->getExchange(), *std::get<0>(it)->getSecurity(), std::get<0>(it)->getSide()*std::get<2>(it));
                }

                DepthEventMsg msg(tt, exchange_ptr, mt, security_ptr, ob, tokens[2] == "BUY_SIDE_UPDATE" ? 1 : -1, stod(tokens[6]), stod(tokens[7]));
                order_vector = strategy->onDepth(msg);
            }

            // Append order to the back of the current order
            if (!order_vector.empty()) {
                for (auto&& it : order_vector) {
                    map<MarketType, double> avail_buying_pwr = {{MarketType::Spot, user.getCapital(MarketType::Spot)},{MarketType::Futures, user.getCapital(MarketType::Futures)}};
                    avail_buying_pwr[it->getMarketType()] -= it->getBaseCurrencySize() * it->getPrice();
                    if (avail_buying_pwr[it->getMarketType()] < 0) {
                        throw std::runtime_error("Submitted order exceeds the available buying power of " + to_string(avail_buying_pwr[it->getMarketType()]));
                        it->rejectOrder();
                    } else {
                        current_orders.push_back(it);
                        orderlog.addOrder(it);
                    }
                }
            }

            // Work with orders
            for (auto&& it : current_orders) {
                if (it->getOrderState() == OrderState::SentToExchange) {
                    it->checkOrderReceived(*tt);
                }
                if (it->isLiveOrder() && (it->getOrderType() == "STOP" || it->getOrderType() == "STOPLIMIT")) {
                    it->checkTriggered(last_traded_price[make_pair(mt, security_ptr)]);
                }
                if (it->checkFillability(ob->getBestBid(),ob->getBestAsk())) {
                    pair<std::shared_ptr<Order>, vector<pair<double, double>>> fills;

                    if (it->getOrderType() == "MARKET" || it->getOrderType() == "STOP") {
                        fills = ob->fillMarketOrder(it);
                    } else if (it->getOrderType() == "LIMIT" || it->getOrderType() == "STOPLIMIT") {
                        double qty_fillable = min(ob->getLimitInstantFillQuantity(it->getPrice(), it->getSide()), it->getLeverageAdjustedBaseCurrencySize());
                        if (qty_fillable != 0) { fills = ob->instantFillLimit(it, qty_fillable); }
                        if (qty_fillable < it->getLeverageAdjustedBaseCurrencySize()) {
                            ob->addOrder(it->getPrice(), it->getSide(), it->getLeverageAdjustedBaseCurrencySize() - qty_fillable, it);
                        }
                    }

                    for (auto& fill_pair : fills.second) {
                        it->fillOrder(fill_pair.second, fill_pair.first);
                    
                        if (it->getSide() == 1) {
                            strategy->updatePosition(it->getMarketType(), *exchange_ptr, *security_ptr, fill_pair.second);
                        } else {
                            strategy->updatePosition(it->getMarketType(), *exchange_ptr, *security_ptr, -fill_pair.second);
                        }

                        Trade* trade = new Trade(it, tt, it->getSide(), fill_pair.second, fill_pair.first, false);
                        tradelog.addTrade(std::shared_ptr<Trade>(trade));

                        user.updateBalance(it->getMarketType(), -trade->getSide() * fill_pair.first * fill_pair.second - trade->getFee());
                        }
                }
            }

            // Remove not-live orders
            for (auto it = current_orders.begin(); it != current_orders.end();) {
                if (!(*it)->isLiveOrder() && !((*it)->getOrderState() == OrderState::SentToExchange)) {
                    it = current_orders.erase(it);
                } else {
                    ++it;
                }
            }

            // Record balance history
            if (tradelog.getTrades().empty()) {
                tradelog.addBalanceHistory(tt, user.getCapital(MarketType::Spot), user.getCapital(MarketType::Futures));
            } else {
                double spot_cap = user.getCapital(MarketType::Spot);
                double futures_cap = user.getCapital(MarketType::Futures);

                for (const auto& pair : strategy->getPositionMap()) {
                    if (pair.second != 0) {
                        if (std::get<0>(pair.first) == MarketType::Spot) {
                            double pos = strategy->getPosition(MarketType::Spot, *exchange_ptr, *security_ptr);
                            spot_cap += pos * (tradelog.computeWeightedAverageFillPrice(abs(pos)) - last_traded_price[{MarketType::Spot, security_ptr}]);
                        } else {
                            double pos = strategy->getPosition(MarketType::Futures, *exchange_ptr, *security_ptr);
                            spot_cap += pos * (tradelog.computeWeightedAverageFillPrice(abs(pos)) - last_traded_price[{MarketType::Futures, security_ptr}]);
                        }
                    }
                }

                tradelog.addBalanceHistory(tt, spot_cap, futures_cap);
            }
        }
    }

    /**
     * Run latency analysis.
     * 
     * Latency = [0, 10, 25, 50, 100, 200, 500, 1000] ns
     */
    void run_latency_analysis(const string& data_path, const string& output_path) {
        map<MarketType, double> initial_buying_power = {{MarketType::Spot, user.getCapital(MarketType::Spot)}, {MarketType::Futures, user.getCapital(MarketType::Futures)}};
        vector<int> latency_values = {0, 10, 25, 50, 100, 200, 500, 1000};

        for (auto latency : latency_values) {
            Clear(initial_buying_power);    // Reset

            // Set latency value
            for (auto exchange : user.getExchanges()) {
                exchange->setSendingLatency(latency);
            }

            runBacktest(data_path);
            latency_analysis_pnl.emplace_back(make_pair(latency, make_pair(tradelog.getBalanceHistory().back().second.first, tradelog.getBalanceHistory().back().second.second)));
        }

        std::ofstream outfile(output_path);
        outfile << std::fixed << std::setprecision(2);

        if (outfile) {
            outfile << "LATENCY,SPOT_BALANCE,FUTURES_BALANCE" << "\n";
            for (const auto& it : latency_analysis_pnl) {
                outfile << it.first << "," << it.second.first << "," << it.second.second << "\n";
            }
            outfile.close();
            std::cout << "Data exported to " << output_path << std::endl;
        } else {
            throw runtime_error("Error opening file for writing");
        }
    }

    /**
     * Getter for trade log.
     */
    TradeLog& getTradeLog() {return tradelog;}

    private:
    User user;
    Strategy* strategy;
    MarketMap orderbooks;
    OrderLog orderlog;
    TradeLog tradelog;
    map<pair<MarketType, std::shared_ptr<Security>>, double> last_traded_price;
    vector<pair<int, pair<double, double>>> latency_analysis_pnl;

    std::shared_ptr<OrderBook> getOrderbook(MarketType market_type, const Exchange& exchange, const Security& security) {
        MarketKey key = make_tuple(market_type, exchange, security);
            auto it = orderbooks.find(key);

            if (it != orderbooks.end()) {
                return it->second;
            }

            throw invalid_argument("Orderbook not found");
            return nullptr;
    }
};
