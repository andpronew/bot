#pragma once
#include "binance_client.hpp"
#include <string>

using namespace std;

class LadderStrategy
{
public:
    LadderStrategy(BinanceClient& client,
                   const string& symbol,
                   int levels,
                   double step,
                   double order_size);

    void run();

private:
    BinanceClient& client_;
    string symbol_;
    int ladder_size_;
    double ladder_step_;
    double order_size_;

    double adjust_order_size(double requested);
};
