#pragma once
#include <string>
#include <cstdint>

using namespace std;

class BinanceClient
{
public:
    BinanceClient(const string& api_key, const string& secret_key, bool sandbox = true);

    double get_price(const string& symbol);

    string place_order(const string& symbol,
                       const string& side,
                       const string& type,
                       double price,
                       double quantity,
                       const string& timeInForce = "GTC");

    void place_limit_order(const string& symbol,
                           const string& side,
                           double price,
                           double quantity);

    string get_open_orders(const string& symbol);
    void print_compact_orders(const string& symbol, int64_t filter_order_id = 0);

    string get_order_history(const string& symbol);

private:
    string api_key_;
    string secret_key_;
    bool sandbox_ = true;
    string base_url_;

    string sign_request(const string& query_string);
    string hmac_sha256(const string& key, const string& data);
    long long current_timestamp_ms();
};
