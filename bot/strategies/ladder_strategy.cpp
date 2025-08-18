#include "ladder_strategy.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

LadderStrategy::LadderStrategy(BinanceClient& client,
                               const string& symbol,
                               int levels,
                               double step,
                               double order_size)
    : client_(client), symbol_(symbol),
      ladder_size_(levels), ladder_step_(step), order_size_(order_size)
{
    cout << "LadderStrategy created with order_size = " << order_size_ << endl;
    cout << "Starting ladder strategy in sandbox mode..." << endl;
}

double LadderStrategy::adjust_order_size(double requested)
{
    // simple passthrough for now; can be improved to obey stepSize/minQty
    return requested;
}

void LadderStrategy::run()
{
    cout << "LadderStrategy running for symbol: " << symbol_ << endl;
    while (true) {
        double price = client_.get_price(symbol_);
        if (price <= 0) {
            cerr << "Failed to get price for " << symbol_ << endl;
            this_thread::sleep_for(chrono::seconds(3));
            continue;
        }
        cout << "Current price: " << price << endl;
        cout << "Placing ladder orders:" << endl;

        for (int i = 1; i <= ladder_size_; ++i) {
            double buy_price = price - i * ladder_step_;
            double sell_price = price + i * ladder_step_;
            double safe_order_size = adjust_order_size(order_size_);

            cout << "  BUY order at " << buy_price << ", size " << safe_order_size << endl;
            string buy_resp = client_.place_order(symbol_, "BUY", "LIMIT", buy_price, safe_order_size);
            if (!buy_resp.empty()) {
                cout << "Full order response: " << buy_resp << endl;
                try {
                    auto j = json::parse(buy_resp);
                    if (j.contains("orderId")) {
                        cout << "Order placed successfully. OrderId: " << j["orderId"] << endl;
                    } else if (j.contains("code") && j.contains("msg")) {
                        cout << "Error placing order: " << j["msg"] << endl;
                    }
                } catch (...) {}
            }

            cout << "  SELL order at " << sell_price << ", size " << safe_order_size << endl;
            string sell_resp = client_.place_order(symbol_, "SELL", "LIMIT", sell_price, safe_order_size);
            if (!sell_resp.empty()) {
                cout << "Full order response: " << sell_resp << endl;
                try {
                    auto j = json::parse(sell_resp);
                    if (j.contains("orderId")) {
                        cout << "Order placed successfully. OrderId: " << j["orderId"] << endl;
                    } else if (j.contains("code") && j.contains("msg")) {
                        cout << "Error placing order: " << j["msg"] << endl;
                    }
                } catch (...) {}
            }
        }

        cout << "----" << endl;
        this_thread::sleep_for(chrono::seconds(3));
    }
}
