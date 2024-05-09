#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../data/exchange.h"
#include "../record/orderlog.h"
#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"

using namespace std;

/**
 * @todo (Reach goal) Add restristictions per subaccount
 * @todo (Reach goal) Manage risks/positions per subaccounts/users for a firm (tree structure)
 */
class User {
public:
    /**
     * Default constructor.
     */
    User() {}

    /**
     * Constructor for user
     */
    User(double spot_balance_, double futures_balance_, const string& json_file_path) {
        id = ++next_id; // Assign a unique ID

        buying_power[MarketType::Spot] = spot_balance_;
        buying_power[MarketType::Futures] = futures_balance_;

        FILE* json_file = fopen(json_file_path.c_str(), "r");
        if (!json_file) {
            throw runtime_error("Error opening Json file: " + json_file_path);
            return;
        }

        char buffer[65536];
        FileReadStream fs(json_file, buffer, sizeof(buffer));
        Document data;
        data.ParseStream(fs);
        fclose(json_file);

        for (auto& exchange : data.GetObject()) {
            Exchange* exchange_ = new Exchange(exchange.name.GetString());
            exchange_->loadJson(json_file_path);
            exchange_list.push_back(std::shared_ptr<Exchange>(exchange_));
        }
    }

    /**
     * Destructor for user
     */
    ~User() {}

    /**
     * Clear/Reset user's buying power
     */
    void Clear(map<MarketType, double> initial_buying_power) {
        buying_power = initial_buying_power;
    }

    /**
     * Getter for user id
     */
    int getId() const { return id; }

    /**
     * Getter for buying power.
     */
    double getCapital(MarketType market_type) const { return buying_power.at(market_type);}

    /**
     * Update buying power.
     */
    void updateBalance(MarketType market_type, double change) {
        buying_power[market_type] += change;
    }

    /**
     * Getter for user's list of exchanges
     */
    vector<std::shared_ptr<Exchange>> getExchanges() const { return exchange_list; }

    /**
     * Find exchange by name, case insensitive
     * @param name name of the exchange to find
     * @return pointer to the exchange
     */
    std::shared_ptr<Exchange> findExchange(const string& name) {
    // Convert the target name to lowercase for case-insensitive comparison
    string lowercase_name = name;
    transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), ::tolower);

        for (auto&& exchange : exchange_list) {
            string exchangeName = exchange->getName();
            transform(exchangeName.begin(), exchangeName.end(), exchangeName.begin(), ::tolower);
            if (exchangeName == lowercase_name) {
                return exchange;
            }
        }

        return nullptr; // Exchange not found
    }

    private:
    int id; /*< Unique user id */
    map<MarketType, double> buying_power; /*< Buying power*/
    static int next_id; /*< Static member to track the next available ID */ 
    vector<std::shared_ptr<Exchange>> exchange_list; /*< List of exchanges for the user */
};

int User::next_id = 0;