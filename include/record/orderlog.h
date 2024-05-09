#pragma once

#include <stdexcept>
#include <vector>
#include "../backtesting/order.h"

using namespace std;

/**
 * Class for order log
 */
class OrderLog {
public:
    /**
     * Default constructor
     */
    OrderLog() {}

    /**
     * Destructor
     */
    ~OrderLog() {}

    /**
     * Clear/Reset
     */
    void Clear() {
        orders.clear();
    }

    /**
     * Add order to the order list
     * @param order order to add
     */
    void addOrder(std::shared_ptr<Order> order) {
        orders.push_back(order);
    }

    /**
     * Getter for  Orders
     */
    vector<std::shared_ptr<Order>> getOrders() const {return orders;}

private:
    vector<std::shared_ptr<Order>> orders;
};