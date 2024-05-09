#pragma once

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include "../backtesting/trade.h"

using namespace std;

/**
 * Class for trade log
 */
class TradeLog {
public:
    /**
     * Default constructor
     */
    TradeLog() {}

    /**
     * Destructor
     */
    ~TradeLog() {}

    /**
     * Clear/Reset
     */
    void Clear() {
        trades.clear();
        balance_history.clear();
    }

    /**
     * Add trade to the trade list
     * @param trade trade to add
     */
    void addTrade(std::shared_ptr<Trade> trade) {
        trades.push_back(trade);
    }

    /**
     * Getter for trades
     */
    vector<std::shared_ptr<Trade>> getTrades() const {return trades;}

    /**
     * Getter for balance history
     */
    vector<pair<std::shared_ptr<TimeType>, pair<double, double>>> getBalanceHistory() const {
        return balance_history;
    }

    /**
     * Add to balance history
     */
    void addBalanceHistory(std::shared_ptr<TimeType> tt, double spot_bal, double futures_bal) {
        balance_history.emplace_back(make_pair(tt, make_pair(spot_bal, futures_bal)));
    }

    /**
     * Compute P&L.
     */
    double computeTotalRealizedPNL() const {
        double pnl = 0;

        for (auto& it : trades) {
            pnl -= it->getSide() * it->getPrice() * it->getBaseCurrencySize() + it->getFee();
        }

        return pnl;
    }

    double computeWeightedAverageFillPrice(double size) const {
        double checkedSize = 0.0;
        double weightedSum = 0.0;

        for (auto it = trades.rbegin(); it != trades.rend(); ++it) {
        const auto& trade = *it;
        double tradeSize = min(trade->getBaseCurrencySize(), size-checkedSize);
        double tradePrice = trade->getPrice();

        // Accumulate checked size
        checkedSize += tradeSize;

        // Update the weighted sum
        weightedSum += tradeSize * tradePrice;

        // Stop when checked size reaches the specified size
        if (checkedSize == size) {
            break;
        }
    }

    // Calculate the weighted average
    double weightedAverage = weightedSum / checkedSize;
    return weightedAverage;
    }

    /**
     * Function to compute Trading pairs
     */
    // vector<pair<vector<pair<std::shared_ptr<Trade>, double>>, vector<pair<std::shared_ptr<Trade>, double>>>> getTradePairs() {

    // }

    /**
     * Export balance history to CSV format
     */
    void exportBalanceHistoryToCSV(const string& filename) {
        std::ofstream outfile(filename);
        outfile << std::fixed << std::setprecision(2);

        if (outfile) {
            outfile << "TIMESTAMP,SPOT_BALANCE,FUTURES_BALANCE" << "\n";
            for (const auto& it : balance_history) {
                outfile << it.first->toString() << "," << it.second.first << "," << it.second.second << "\n";
            }
            outfile.close();
            std::cout << "Data exported to " << filename << std::endl;
        } else {
            std::cerr << "Error opening file for writing." << std::endl;
        }
    }

    /**
     * Export tradelog to CSV format
     */
    void exportTradeLogToCSV(const string& filename) {
        std::ofstream outfile(filename);
        outfile << std::fixed << std::setprecision(2);

        if (outfile) {
            outfile << "TIMESTAMP,SECURITY,MARKET_TYPE,EXCHANGE,SIDE,SIZE,FEE" << "\n";
            for (const auto& it : trades) {
                string s = (it->getSide() == 1) ? "Buy" : "sell";
                outfile << it->getTimestamp()->toString() << "," << *(it->getSecurity()) << "," << it->getParentOrder()->getMarketType() << "," << it->getExchange()->getName() 
                        << "," << s << "," << it->getBaseCurrencySize() << "," << it->getFee() << "\n";
            }
            outfile.close();
            std::cout << "Data exported to " << filename << std::endl;
        } else {
            std::cerr << "Error opening file for writing." << std::endl;
        }
    }


private:
    vector<std::shared_ptr<Trade>> trades;      /*< Vector of pointers to trades */
    vector<pair<std::shared_ptr<TimeType>, pair<double, double>>> balance_history;      /*< Vector recording real time balance */
};