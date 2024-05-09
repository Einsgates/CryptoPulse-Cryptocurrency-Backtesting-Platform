## Binance
- [Api Docs](https://docs.binance.us/)
- [Create New Order](https://docs.binance.us/#get-order-rate-limits-user_data)

## OKX
- [Create New Order](https://www.okx.com/docs-v5/en/#order-book-trading-trade)

## Coinbase
- [HFT API](https://docs.cloud.coinbase.com/exchange/docs/fix-msg-order-entry-50)

## ByBit
- [Place Order](https://bybit-exchange.github.io/docs/v5/order/create-order)

### Make order

| Parameter | Required | Type | Comments |
|-----------|----------|------|----------|
| category | true | string | Product type Unified account: spot, linear, inverse, option Classic account: spot, linear, inverse |
| symbol | true | string | Symbol name |
| isLeverage | false | integer | Whether to borrow. Valid for Unified spot only. 0(default): false then spot trading, 1: true then margin trading |
| side | true | string | Buy, Sell |
| orderType | true | string | Market, Limit |
| qty | true | string | Order quantity UTA account Spot: set marketUnit for market order qty unit, quoteCoin for market buy by default, baseCoin for market sell by default Perps, Futures & Option: always use base coin as unit Classic account Spot: the unit of qty is quote coin for market buy order, for others, it is base coin Perps, Futures: always use base coin as unit Perps & Futures: if you pass qty="0" and reduceOnly="true", you can close the whole position of current symbol |
| marketUnit | false | string | The unit for qty when create Spot market orders for UTA account baseCoin: for example, buy BTCUSDT, then "qty" unit is BTC quoteCoin: for example, sell BTCUSDT, then "qty" unit is USDT |
| price | false | string | Order price Market order will ignore this field Please check the min price and price precision from instrument info endpoint If you have position, price needs to be better than liquidation price |
| triggerDirection | false | integer | Conditional order param. Used to identify the expected direction of the conditional order. 1: triggered when market price rises to triggerPrice 2: triggered when market price falls to triggerPrice Valid for linear & inverse |
| orderFilter | false | string | If it is not passed, Order by default. Order tpslOrder: Spot TP/SL order, the assets are occupied even before the order is triggered StopOrder: Spot conditional order, the assets will not be occupied until the price of the underlying asset reaches the trigger price, and the required assets will be occupied after the Conditional order is triggered Valid for spot only |
| triggerPrice | false | string | For Perps & Futures, it is the conditional order trigger price. If you expect the price to rise to trigger your conditional order, make sure: triggerPrice > market price Else, triggerPrice < market price For spot, it is the TP/SL and Conditional order trigger price |
| triggerBy | false | string | Trigger price type, Conditional order param for Perps & Futures. LastPrice, IndexPrice, MarkPrice Valid for linear & inverse |
| orderIv | false | string | Implied volatility. option only. Pass the real value, e.g for 10%, 0.1 should be passed. orderIv has a higher priority when price is passed as well |
| timeInForce | false | string | Time in force Market order will use IOC directly If not passed, GTC is used by default |
| positionIdx | false | integer | Used to identify positions in different position modes. Under hedge-mode, this param is required (USDT perps & Inverse contracts have hedge mode) 0: one-way mode 1: hedge-mode Buy side 2: hedge-mode Sell side |
| orderLinkId | false | string | User customised order ID. A max of 36 characters. Combinations of numbers, letters (upper and lower cases), dashes, and underscores are supported. Futures & Perps: orderLinkId rules: optional param always unique option orderLinkId rules: required param always unique |
| takeProfit | false | string | Take profit price linear & inverse: support UTA and classic account spot(UTA): Spot Limit order supports take profit, stop loss or limit take profit, limit stop loss when creating an order |
| stopLoss | false | string | Stop loss price linear & inverse: support UTA and classic account spot(UTA): Spot Limit order supports take profit, stop loss or limit take profit, limit stop loss when creating an order |
| tpTriggerBy | false | string | The price type to trigger take profit. MarkPrice, IndexPrice, default: LastPrice. Valid for linear & inverse |
| slTriggerBy | false | string | The price type to trigger stop loss. MarkPrice, IndexPrice, default: LastPrice. Valid for linear & inverse |
| reduceOnly | false | boolean | What is a reduce-only order? true means your position can only reduce in size if this order is triggered. You must specify it as true when you are about to close/reduce the position When reduceOnly is true, take profit/stop loss cannot be set Valid for linear, inverse & option |
| closeOnTrigger | false | boolean | What is a close on trigger order? For a closing order. It can only reduce your position, not increase it. If the account has insufficient available balance when the closing order is triggered, then other active orders of similar contracts will be cancelled or reduced. It can be used to ensure your stop loss reduces your position regardless of current available margin. Valid for linear & inverse |
| smpType | false | string | Smp execution type. What is SMP? |
| mmp | false | boolean | Market maker protection. option only. true means set the order as a market maker protection order. What is mmp? |
| tpslMode | false | string | TP/SL mode Full: entire position for TP/SL. Then, tpOrderType or slOrderType must be Market Partial: partial position tp/sl. Limit TP/SL order are supported. Note: When create limit tp/sl, tpslMode is required and it must be Partial Valid for linear & inverse |
| tpLimitPrice | false | string | The limit order price when take profit price is triggered linear & inverse: only works when tpslMode=Partial and tpOrderType=Limit Spot(UTA): it is required when the order has takeProfit and tpOrderType=Limit |
| slLimitPrice | false | string | The limit order price when stop loss price is triggered linear & inverse: only works when tpslMode=Partial and slOrderType=Limit Spot(UTA): it is required when the order has stopLoss and slOrderType=Limit |
| tpOrderType | false | string | The order type when take profit is triggered linear & inverse: Market(default), Limit. For tpslMode=Full, it only supports tpOrderType=Market Spot(UTA): Market: when you set "takeProfit", Limit: when you set "takeProfit" and "tpLimitPrice" |
| slOrderType | false | string | The order type when stop loss is triggered linear & inverse: Market(default), Limit. For tpslMode=Full, it only supports slOrderType=Market Spot(UTA): Market: when you set "stopLoss", Limit: when you set "stopLoss" and "slLimitPrice" |