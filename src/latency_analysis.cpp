#include <iostream>
#include <iomanip>

#include "../include/backtesting/order.h"
#include "../include/backtesting/trade.h"
#include "../include/backtesting/backtester.h"
#include "../include/data/exchange.h"
#include "../include/data/security.h"
#include "./sample_strategy.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << "<Initial Spot Balance> <Initial Futures Balance> <Configuration Path> <Market Data Path>\n";
    }

    User user(stod(argv[1]), stod(argv[2]), argv[3]);

    MovingAverageCross ma_cross(user, 180, 5, 20);  /*< Your straetgy class constructor */

    Backtester backtester(user, &ma_cross);     /*< Replace "&ma_cross" with your strategy instance*/
    backtester.run_latency_analysis(argv[4], "./sample_data/sample_latency_analysis.csv");
}