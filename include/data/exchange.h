#pragma once

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "security.h"
#include "util.h"
#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"

enum class MarketType;

using namespace rapidjson;
using namespace std;


/**
 * Abstract class for exchagnes.
 * @todo: Have a method to configure latency per each day
 */
class Exchange {
    public:
        /**
         * Default constructor, used to indicate "ALL EXCHANGES"
         */
        Exchange(): name("") {}

        /**
         * Constructor to create an exchange
         */
        Exchange(const string& name_): name(name_) {}

        /**
         * Destructor
         */
        virtual ~Exchange() {}

        /**
         * Reads json file and stores latencies, trade rules, and fee structures.
         * @param filename Filename of the Json file to read.
        */
        bool loadJson(const string& json_file_path) {
            FILE* json_file = fopen(json_file_path.c_str(), "r");
            if (!json_file) {
                throw runtime_error("Error opening Json file: " + json_file_path);
                return false;
            }

            char buffer[65536];
            FileReadStream fs(json_file, buffer, sizeof(buffer));
            Document data;
            data.ParseStream(fs);
            fclose(json_file);

            // Case-insensitive search for the exchange name
            for (auto& exchange : data.GetObject()) {
                if (exchange.name.GetString() == name) {
                    // Extract latency values
                    sending_latency = exchange.value["nanosecondLatencyTo"].GetInt();
                    receiving_latency = exchange.value["nanosecondLatencyFrom"].GetInt();

                    /**
                     * Extract Trading Rules
                     */
                    // Spot/Margin
                    const Value& spot_margin_rules = exchange.value["tradeingRules"]["Spot/Margin"];
                    for (auto& security : spot_margin_rules.GetObject()) {
                        string security_str = security.name.GetString();
                        size_t slash_pos = security_str.find('/');
                        Security* sec = new Security(security_str.substr(0, slash_pos), security_str.substr(slash_pos + 1));
                        listed_securities[MarketType::Spot].push_back(std::shared_ptr<Security>(sec));

                        // Save rule values
                        const Value& rule_values = security.value;
                        vector<double> rule_vector;
                        for (auto& v : rule_values.GetArray()) {
                            rule_vector.push_back(v.GetDouble());
                        }
                        trading_rules[MarketType::Spot][*sec] = rule_vector;
                    }

                    // Futures
                    const Value& futures_rules_rules = exchange.value["tradeingRules"]["Futures"];
                    for (auto& security : futures_rules_rules.GetObject()) {
                        string security_str = security.name.GetString();
                        size_t slash_pos = security_str.find('/');
                        Security* sec = new Security(security_str.substr(0, slash_pos), security_str.substr(slash_pos + 1));
                        listed_securities[MarketType::Futures].push_back(std::shared_ptr<Security>(sec));

                        // Save rule values
                        const Value& rule_values = security.value;
                        vector<double> rule_vector;
                        for (auto& v : rule_values.GetArray()) {
                            rule_vector.push_back(v.GetDouble());
                        }
                        trading_rules[MarketType::Futures][*sec] = rule_vector;
                    }

                    /**
                     * Extract Fee Structures
                     */

                    // Spot/Margin
                    const Value& spotMakerFees = exchange.value["feeStructure"]["Spot/Margin"]["Maker"];
                    const Value& spotTakerFees = exchange.value["feeStructure"]["Spot/Margin"]["Taker"];

                    for (SizeType i = 0; i < spotTakerFees.Size(); ++i) {
                        trading_fee_schedule[MarketType::Spot].emplace_back(spotMakerFees[i].GetDouble(), spotTakerFees[i].GetDouble());
                    }

                    maker_fee[MarketType::Spot] = trading_fee_schedule[MarketType::Spot][0].first; // Set maker fee by default
                    taker_fee[MarketType::Spot] = trading_fee_schedule[MarketType::Spot][0].second; // Set taker fee by default

                    // Futures
                    const Value& futuresMakerFees = exchange.value["feeStructure"]["Futures"]["Maker"];
                    const Value& futuresTakerFees = exchange.value["feeStructure"]["Futures"]["Taker"];

                    for (SizeType i = 0; i < futuresTakerFees.Size(); ++i) {
                        trading_fee_schedule[MarketType::Futures].emplace_back(futuresMakerFees[i].GetDouble(), futuresTakerFees[i].GetDouble());
                    }

                    maker_fee[MarketType::Futures] = trading_fee_schedule[MarketType::Futures][0].first; // Set maker fee by default
                    taker_fee[MarketType::Futures] = trading_fee_schedule[MarketType::Futures][0].second; // Set taker fee by default

                    return true; // Found the exchange
                }
            }

            throw invalid_argument("Exchange configuration not found");
            return false;
        }


        /**
         * Getter for the trade rules of the security within the exchange.
         * @param sec Security that we would like the trade rules.
        */
        vector<double> getTradingRules(MarketType market_type, const Security& sec) const {
            auto it = trading_rules.at(market_type).find(sec);
            if (it != trading_rules.at(market_type).end()) {
                return it->second;
            }

            // Could not find tick size
            throw invalid_argument("Trading Rule not found for given security.");
            return vector<double>();
        }

        /**
         * Getter for the exchange name.
         */
        string getName() const {return name;}

        /**
         * Getter for the receiving latency.
         */
        int getReceivingLatency() const {return receiving_latency;}

        /**
         * Getter for the sending latency.
         */
        int getSendingLatency()const {return sending_latency;}

        /**
         * Setter for the receiving latency.
         */
        void setReceivingLatency(int latency) {receiving_latency = latency;}

        /**
         * Setter for the sending latency.
         */
        void setSendingLatency(int latency) {sending_latency = latency;}

        /**
         * Getter for maker fee.
         */
        double getMakerFee(MarketType market_type) const {return maker_fee.at(market_type);}

        /**
         * Getter for taker fee.
         */
        double getTakerFee(MarketType market_type) const {return taker_fee.at(market_type);}

        /**
         * Getter for listed securities
         * @param market_type type of the market (Futures, Spot)
         */
        const vector<std::shared_ptr<Security>>& getListedSecurities(MarketType market_type) const {
            auto it = listed_securities.find(market_type);
            
            if (it != listed_securities.end()) {
                return it->second; // Return a const reference to the vector
            } else {
                static vector<std::shared_ptr<Security>> emptyVector;
                return emptyVector;
            }
        }

        /**
         * Search for the secuity with provided name
         */
        std::shared_ptr<Security> findSecurity(MarketType market_type, const std::string& name) const {
            auto it = listed_securities.find(market_type);
            if (it != listed_securities.end()) {
                // Search within the vector for the specified security name
                for (auto&& security : it->second) {
                    if (security->getBase() + "/" + security->getQuote() == name) {
                        return security; // Return a pointer to the found security
                    }
                }
            }
            return nullptr; // Security not found
        }

        /**
         * Setter for trading fee from fee schedule.
        */
        void setTradingFeeFromSchedule(MarketType market_type, int level) {
            if (level >= trading_fee_schedule[market_type].size()) {
                throw invalid_argument("Invalid customer level of " + to_string(level));
                return;
            }

            maker_fee[market_type] = trading_fee_schedule[market_type][level].first;
            taker_fee[market_type] = trading_fee_schedule[market_type][level].second;
        }

        /**
         * Setter for maker fee.
         * Manually set the fee outside the provided fee structure.
         * @param fee maker fee in percent.
         */
        void setMakerFee(MarketType market_type, double fee) {maker_fee[market_type] = fee;}

        /**
         * Setter for taker fee.
         * Manually set the fee outside the provided fee structure.
         * @param fee taker fee in percent.
         */
        void setTakerFee(MarketType market_type, double fee) {taker_fee[market_type] = fee;}

        /**
         * Equal operator.
         */
        bool operator==(const Exchange& other) const {
            return name == other.name;
        }

        /**
         * Hash used for unordered map in exchange class.
         */
        struct Hash {
            size_t operator()(const Exchange& exchange) const {
                // Use the hash value of the exchange name (case-insensitive)
                return std::hash<std::string>{}(exchange.getName());
            }
        };

    protected:
        const string name;           /*< Exchange name */
        int receiving_latency;       /*< Latency from an exchange in nanoseconds */
        int sending_latency;         /*< Laency to an exchange in nanoseconds */
        map<MarketType, double> maker_fee;  /*< Maker fee in percent */
        map<MarketType, double> taker_fee;  /*< Taker fee in percent */
        map<MarketType, unordered_map<Security, vector<double>, Security::Hash>> trading_rules;  /*< Map storing trade rules*/
        map<MarketType,  vector<pair<double, double>>> trading_fee_schedule;    /*< Trading fee schedule */
        map<MarketType, vector<std::shared_ptr<Security>>> listed_securities;      /*< List of securities listed per market*/
};