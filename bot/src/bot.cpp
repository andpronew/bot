#include "ladder_strategy.h"
#include "binance_client.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

void run_bot()
{
    ifstream config_file("config.json");
    if (!config_file.is_open()) {
        cerr << "Failed to open config.json" << endl;
        return;
    }

    json config;
    config_file >> config;

    string api_key = config.value("api_key", "");
    string secret_key = config.value("secret_key", "");
    bool sandbox = config.value("sandbox", true);
    string symbol = config.value("symbol", "BTCUSDT");
    int ladder_size = config.value("ladder_size", 5);
    double ladder_step = config.value("ladder_step", 1.0);
    double order_size = config.value("order_size", 0.0001);

    BinanceClient client(api_key, secret_key, sandbox);

    LadderStrategy strategy(client, symbol, ladder_size, ladder_step, order_size);

    strategy.run();
}
