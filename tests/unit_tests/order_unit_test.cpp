#include "gtest/gtest.h"
#include "backtesting/order.h"
#include <string>

int Order::next_id=0;


// Create securities
Security btcusdt("BTC", "USDT");

// Create exchanges
Exchange binance("Binance");
Exchange bybit("Bybit");
Exchange coinbase("Coinbase");
Exchange okx("Okx");


TEST(OrderTest, SanityCheckTest) {
// Load configuration Json files
binance.loadJson("./tests/exchange_json_test/exchange.json");
bybit.loadJson("./tests/exchange_json_test/exchange.json");
coinbase.loadJson("./tests/exchange_json_test/exchange.json");
okx.loadJson("./tests/exchange_json_test/exchange.json");

// Testing invalid order side
Order wrong_order_side(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 0, 1.5, 0, 1, MarginType::NoMargin, 70000.5, binance);
EXPECT_EQ(wrong_order_side.getOrderState(), OrderState::Rejected);

// Testing zero order size
Order zero_size_order(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 0, 0, 1, MarginType::NoMargin, 70000.5, binance);
EXPECT_EQ(zero_size_order.getOrderState(), OrderState::Rejected);

// Testing negative base order size
Order negative_base_size(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, -1, 0, 1, MarginType::NoMargin, 70000.5, binance);
EXPECT_EQ(negative_base_size.getOrderState(), OrderState::Rejected);

// Testing negative quote order size
Order negative_quote_size(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 0, -1000, 1, MarginType::NoMargin, 70000.5, binance);
EXPECT_EQ(negative_quote_size.getOrderState(), OrderState::Rejected);

// Testing both base and quote order size
Order both_order_size_provided(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 1000, 1, MarginType::NoMargin, 70000.5, binance);
EXPECT_EQ(both_order_size_provided.getOrderState(), OrderState::Rejected);

// Testing invalid minimum tick size
Order invalid_minimum_tick(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 0, 1, MarginType::NoMargin, 70000.512, binance);
EXPECT_EQ(invalid_minimum_tick.getOrderState(), OrderState::Rejected);

// Testing sub 1 leverage
Order sub_one_leverage(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 0, 0.9, MarginType::Cross, 70000.51, binance);
EXPECT_EQ(sub_one_leverage.getOrderState(), OrderState::Rejected);

// Testing no margin
Order no_margin(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 0, 1, MarginType::Cross, 70000.51, binance);
Order leveraged_no_margin(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 0, 2, MarginType::NoMargin, 70000.51, binance);
EXPECT_EQ(no_margin.getOrderState(), OrderState::Rejected);
EXPECT_EQ(leveraged_no_margin.getOrderState(), OrderState::Rejected);

// Testing maximum leverage
Order isolated_leverage(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 0, 11, MarginType::Isolated, 70000.51, binance);
Order cross_leverage(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 1.5, 0, 6, MarginType::Cross, 70000.51, binance);
EXPECT_EQ(isolated_leverage.getOrderState(), OrderState::Rejected);
EXPECT_EQ(cross_leverage.getOrderState(), OrderState::Rejected);

// Testing minimum order size
Order base_min_order(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 0.000001, 0, 5, MarginType::Isolated, 70000.51, binance);
Order quote_min_order(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 0, 0.9, 5, MarginType::Isolated, 70000.51, binance);
EXPECT_EQ(base_min_order.getOrderState(), OrderState::Rejected);
EXPECT_EQ(quote_min_order.getOrderState(), OrderState::Rejected);

// Testing maximum limit order size
Order limit_base_max(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 2000, 0, 5, MarginType::Isolated, 70000.51, binance);
Order limit_quote_max(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 0, 500000, 5, MarginType::Isolated, 70000.51, bybit);
EXPECT_EQ(limit_base_max.getOrderState(), OrderState::Rejected);
EXPECT_EQ(quote_min_order.getOrderState(), OrderState::Rejected);

// Testing maximum market order size
Order market_base_max(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "MARKET", 1, 15, 0, 5, MarginType::Isolated, 70000.51, binance);
Order market_quote_max(btcusdt, MarketType::Spot, "2023-12-31 23:59:59.999999999", "MARKET", 1, 0, 500000, 5, MarginType::Isolated, 70000.51, bybit);
EXPECT_EQ(market_base_max.getOrderState(), OrderState::Rejected);
EXPECT_EQ(market_quote_max.getOrderState(), OrderState::Rejected);
}

TEST(LimitOrderTest, ConstructorTest) {
// Create a Limit Order object
Limit limit(btcusdt, MarketType::Spot,"2023-12-31 23:59:59.999999999", 1, 10, 0, 1, MarginType::NoMargin, 10.0, binance);

// Check that the Order object was created correctly
EXPECT_EQ(limit.getSecurity(), btcusdt);
EXPECT_EQ(limit.getMarketType(), MarketType::Spot);
EXPECT_EQ(limit.getTimestamp(), "2023-12-31 23:59:59.999999999");
EXPECT_EQ(limit.getOrderType(), "LIMIT");
EXPECT_EQ(limit.getSide(), 1);
EXPECT_EQ(limit.getBaseCurrencySize(), 10.0);
EXPECT_EQ(limit.getQuoteCurrencySize(), 100);
EXPECT_EQ(limit.getLeverage(), 1);
EXPECT_EQ(limit.getMarginType(), MarginType::NoMargin);
EXPECT_EQ(limit.getPrice(), 60850.0);
EXPECT_EQ(limit.getExchange().getName(), binance.getName());
}

TEST(LimitOrderTest, FillabilityTest) {
// Create a Limit Buy Order object
Limit limit_buy(btcusdt, MarketType::Spot,"2023-12-31 23:59:59.999999999", 1, 10, 0, 1, MarginType::NoMargin, 60850.25, binance);

// Check filability (Only should be fillable if best ask is same or lower than the buy price)
EXPECT_FALSE(limit_buy.checkFillability(60850.25, 60850.75));
EXPECT_TRUE(limit_buy.checkFillability(60849.75, 60850.25));

// Create a Limit Sell Order object
Limit limit_sell(btcusdt, MarketType::Spot,"2023-12-31 23:59:59.999999999", -1, 10, 0, 1, MarginType::NoMargin, 60850.25, binance);

// Check filability (Only should be fillable if best bid is same or higher than the buy price)
EXPECT_FALSE(limit_sell.checkFillability(60850.00, 60850.25));
EXPECT_TRUE(limit_sell.checkFillability(60850.25, 60850.75));
}

TEST(LimitOrderTest, FillOrderTest) {
// Create a Limit Buy Order object
Limit limit(btcusdt, MarketType::Spot,"2023-12-31 23:59:59.999999999", 1, 100, 0, 1, MarginType::NoMargin, 10, binance);

limit.fillOrder(1, 11); // Should not fill
EXPECT_TRUE(limit.getBaseCurrencySize() == 10 && limit.getOrderState() == OrderState::Working);

limit.fillOrder(1, 4.5);
EXPECT_TRUE(limit.getBaseCurrencySize() == 10 && limit.getOrderState() == OrderState::PartiallyFilled && limit.getFilledSize() == 4.5 && limit.isLiveOrder());

limit.fillOrder(1, 6);
EXPECT_TRUE(limit.getBaseCurrencySize() == 10 && limit.getFilledSize() == 4.5);

limit.fillOrder(1, 5.5);
EXPECT_TRUE(limit.getBaseCurrencySize() == 10 && limit.getFilledSize() == 10 && limit.getOrderState() == OrderState::Filled && !limit.isLiveOrder());
}

TEST(LimitOrderTest, ModifyOrderTest) {
// Create a Limit Buy Order object
Limit limit(btcusdt, MarketType::Spot,"2023-12-31 23:59:59.999999999", 1, 10, 0, 1, MarginType::NoMargin, 10, binance);

limit.modifyOrder(12, 10, 10);  // Should output cerr
EXPECT_TRUE(limit.getBaseCurrencySize() == 10 && limit.getQuoteCurrencySize() == 100);

limit.modifyOrder(0, 400, 20);  // Should change base currency size to 20
EXPECT_TRUE(limit.getBaseCurrencySize() == 20 && limit.getQuoteCurrencySize() == 400 && limit.getPrice() == 20);

limit.fillOrder(1, 10);
limit.modifyOrder(5, 0, 12);    // Should fail
EXPECT_TRUE(limit.getBaseCurrencySize() == 20 && limit.getQuoteCurrencySize() == 400 && limit.getPrice() == 20 && limit.getFilledSize() == 10);
limit.modifyOrder(0, 190, 12);  // Should fail
EXPECT_TRUE(limit.getBaseCurrencySize() == 20 && limit.getQuoteCurrencySize() == 400 && limit.getPrice() == 20 && limit.getFilledSize() == 10);

limit.modifyOrder(15, 0, 12);
EXPECT_TRUE(limit.getBaseCurrencySize() == 15 && limit.getQuoteCurrencySize() == 180 && limit.getPrice() == 12 && limit.getFilledSize() == 10);
// limit.modifyOrder(0, )
}

// TEST(OrderTest, FillOrderTest) {
// // Create a Security object
// Security security("USD", "AAPL");

// // Create an Exchange object
// Exchange exchange("NYSE", 3, 5);

// // Create an Order object
// Order order(security, "2023-12-31 23:59:59.999999999", "LIMIT", 1, 100.0, 50.0, exchange);

// // Fill the order
// order.fillOrder(50.0);

// // Check that the order was filled correctly
// EXPECT_EQ(order.getFilledSize(), 50.0);
// EXPECT_EQ(order.getOrderState(), OrderState::PartiallyFilled);
// }