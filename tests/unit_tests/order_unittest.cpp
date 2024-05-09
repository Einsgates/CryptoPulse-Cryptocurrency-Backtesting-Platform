#include "gtest/gtest.h"
#include "backtesting/order.h"
#include <string>

// Initialize the static member variable
uint32_t Order::next_id = 1;  // Start IDs at 1

TEST(OrderTest, ConstructorTest) {
// Create a Security object
Security security("USD", "AAPL");

// Create an Exchange object
Exchange exchange("NYSE", 3, 5);

// Create an Order object
Order order(security, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 100.0, 50.0, exchange);

// Check that the Order object was created correctly
EXPECT_EQ(order.getSecurity(), security);
EXPECT_EQ(order.getTimestamp(), "2023-12-31 23:59:59.999999999");
EXPECT_EQ(order.getOrderType(), "LIMIT");
EXPECT_EQ(order.getSide(), 1);
EXPECT_EQ(order.getSize(), 100.0);
EXPECT_EQ(order.getPrice(), 50.0);
EXPECT_EQ(order.getExchange(), exchange);
}

TEST(OrderTest, FillOrderTest) {
// Create a Security object
Security security("USD", "AAPL");

// Create an Exchange object
Exchange exchange("NYSE", 3, 5);

// Create an Order object
Order order(security, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 100.0, 50.0, exchange);

// Fill the order
order.fillOrder(50.0);

// Check that the order was filled correctly
EXPECT_EQ(order.getFilledSize(), 50.0);
EXPECT_EQ(order.getOrderState(), OrderState::PartiallyFilled);
}