# Crypto Backtester Documentation

[[_TOC_]]


## 1. Introduction

The **Crypto Trading Strategy Backtester** is a powerful tool designed to evaluate and analyze trading strategies in the cryptocurrency market. Whether you're a seasoned trader or a beginner, this backtesting framework allows you to fine-tune your strategies, assess their performance, and make informed decisions.

### Key Features:

1. **Flexible Configuration:**
   - Users can easily configure exchange trade rules, fee structures, and latency parameters to match their specific trading environment.
   - Fine-tune your settings to simulate real-world conditions and optimize your strategy.

2. **Automated Backtesting:**
   - Simply feed the historical market data (in CSV format) and your custom C++ strategy file into the backtester.
   - The backtester automatically runs the backtest, providing detailed performance metrics and visualizations.

3. **Latency Analysis:**
   - Gain insights into how latency impacts your strategy's profitability.
   - Run latency analysis experiments to understand how changes in execution speed affect your overall profit and loss (PNL).


## 2. Doxygen 

[Doxygen Documentation](https://crypto-backtesting.static.domains/)

## 3. Data Input Format

### 3.1 Exchange Configuration Json Format
``` cpp
{
    "Exchange 1": {
        /**
         * Latency to exchange in nanoseconds
         * @type int
         */
        "nanosecondLatencyTo": 800,
        
        /**
         * Latency from exchange in nanoseconds
         * @type int
         */
        "nanosecondLatencyFrom": 1000,
        
        /**
         * Trading Rules
         */
         "tradingRules": {
            "Spot/Margin": {
                /**
                 * Indices:
                 * 0: minimum tick size in quote currency
                 * 1: minimum order quantity in base currency
                 * 2: minimum order value in quote currency
                 * 3: maximum limit order quantity in base currency
                 * 4: maximum limit order value in quote currency
                 * 5: maximum market order quantity in base currency
                 * 6: maximum market order value in quote currency
                 * 7: maximum number of open limit order
                 * 8: limit order price limit (%)
                 * 9: market order price limit (%)
                 * 10: maximum isolated leverage (margin)
                 * 11: maximum cross leverage (margin)
                 *
                 * Any empty values should be -1
                 * @type double
                 */
                "Security 1": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
                "Security 2": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
                ...
            },
            "Futures": {
                ...
            }
         },
         "feeStructure": {
             "Spot/Margin": {
                /**
                 * Fee schedule from lowest status to highest in %
                 */
                 "Maker": [0.3, 0.2, 0.1, 0.05],
                 "Taker": [0.5, 0.4, 0.3, 0.2]
             },
             "Futures": {
                 ...
             }
         }
    },
    "Exchange 2": {
        ...
    }
}
```

### 3.2 Market Data CSV Format

 - **COLLECTION_TIME**: UTC datetime when data arrived in format "*yyyy-mm-dd HH:MM:SS.fffffffff*".  
 - **MESSAGE_ID**: Unique id of the message (not required).
 - **MESSAGE_TYPE**: 
	 - 'T' for trade updates.
	 - "BID_UPDATE" or "ASK_UPDATE" for quote updates.
	 - "BUY_SIDE_UPDATE" or "SELL_SIDE_UPDATE" for depth updates.
- **SYMBOL**: Symbol of the instrument in pair notation *(Ex: BTC/USDT)*.
- **MARKET_CENTER**: Market center (exchange) the update took place in.
- **PRICE**:
	- Trade price for trade updates.
	- Updated orderbook level for depth updates.
- **SIZE**:
	- Trade size for trade updates.
	- Updated orderbook size for depth updates.
- **BID_PRICE_1**: best (highest) bid price.
-  **BID_SIZE_1**: corresponding size.
- **BID_PRICE_2**: second highest bid price.
-  **BID_SIZE_2**: corresponding size.
- **BID_PRICE_3**: third highest bid price.
-  **BID_SIZE_3**: corresponding size.
- **ASK_PRICE_1**: best (lowest) ask price.
-  **ASK_SIZE_1**: corresponding size.
- **ASK_PRICE_2**: second lowest ask price.
-  **ASK_SIZE_2**: corresponding size.
- **ASK_PRICE_3**: third lowest ask price.
-  **ASK_SIZE_3**: corresponding size.


## 4. Usage

### 4.1 Running Backtest

1. Follow the comments in /src/backtest.cpp regarding adding your own strategy instance. 

2. Ensure that the script file has the execution permission by running the following command in the terminal:

```console
chmod +x ./run_backtest.sh
```

3. Then run the following command:

```console
./run_backtest.sh <Initial Spot Balance> <Initial Futures Balance> <Configuration Path> <Market Data Path>
```


### 4.2 Running Latency Analysis

1. Follow the comments in /src/latency_analysis.cpp regarding adding your own strategy instance. 

2. Ensure that the script file has the execution permission by running the following command in the terminal:

```console
chmod +x ./run_latency_analysis.sh
```

3. Then run the following command:

```console
./run_latency_analysis.sh <Initial Spot Balance> <Initial Futures Balance> <Configuration Path> <Market Data Path>
```