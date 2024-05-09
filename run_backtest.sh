#!/bin/bash

if [ $# -ne 4 ]; then
    echo "Usage: $0 <Initial Spot Balance> <Initial Futures Balance> <Configuration Path> <Market Data Path>"
    exit 1
fi

arg1="$1"
arg2="$2"
arg3="$3"
arg4="$4"

g++ -std=c++20 -I./boost_1_84_0 ./src/backtest.cpp -o backtest_executable

if [ $? -eq 0 ]; then
    ./backtest_executable "$arg1" "$arg2" "$arg3" "$arg4"
    python3 ./src/backtest.py
else
    echo "Compilation failed. Please check your code."
fi
