#pragma once

#include <cmath>
#include <iostream>
#include <string>

using namespace std;

double round(double num, int digits) {
    double multiplier = std::pow(10.0, digits);
    return std::round(num * multiplier) / multiplier;
}

/**
 * Enumeration class for order state
 * @todo: Add a state where packet is on the way to the exchange
 */
enum class OrderState {
    SentToExchange,
    Working,
    Filled,
    PartiallyFilled,
    Cancelled,
    Rejected
};

/**
 * << operator overload for OrderState class.
 */
ostream& operator<<(ostream& os, const OrderState& state) {
    switch (static_cast<int>(state)) {
        case static_cast<int>(OrderState::SentToExchange):
            os << "Sent to Exchange";
            break;
        case static_cast<int>(OrderState::Working):
            os << "Working";
            break;
        case static_cast<int>(OrderState::Filled):
            os << "Filled";
            break;
        case static_cast<int>(OrderState::PartiallyFilled):
            os << "Partially Filled";
            break;
        case static_cast<int>(OrderState::Cancelled):
            os << "Cancelled";
            break;
        case static_cast<int>(OrderState::Rejected):
            os << "Rejected";
            break;
        default:
            os << "Unknown"; // Handle any other values gracefully
    }
    return os;
}

/**
 * Enumeration class for market type
 */
enum class MarketType {
    Spot,
    Futures
};

/**
 * << operator overload for MarketType class.
 */
ostream& operator<<(ostream& os, const MarketType& market_type) {
    switch (static_cast<int>(market_type)) {
        case static_cast<int>(MarketType::Spot):
            os << "Spot";
            break;
        case static_cast<int>(MarketType::Futures):
            os << "Futures";
            break;
        default:
            os << "Unknown"; // Handle any other values gracefully
    }
    return os;
}

/**
 * Hash overload for MarketType
 */
namespace std {
    template <>
    struct hash<MarketType> {
        size_t operator()(const MarketType& type) const {
            // Return a constant hash value for each enum value
            switch (static_cast<int>(type)) {
                case static_cast<int>(MarketType::Spot):
                    return 42; // Replace with your desired hash value
                case static_cast<int>(MarketType::Futures):
                    return 24; // Replace with another desired hash value
            }
            return -1; // return -1 for unknown values
        }
    };
}

/**
 * Enumeration class for market type
 */
enum class MarginType {
    NoMargin,
    Cross,
    Isolated
};

/**
 * << operator overload for OrderState class.
 */
ostream& operator<<(ostream& os, const MarginType& margin_type) {
    switch (static_cast<int>(margin_type)) {
        case static_cast<int>(MarginType::NoMargin):
            os << "No Margin";
            break;
        case static_cast<int>(MarginType::Isolated):
            os << "Isolated";
            break;
        case static_cast<int>(MarginType::Cross):
            os << "Cross";
            break;
        default:
            os << "Unknown"; // Handle any other values gracefully
    }
    return os;
}