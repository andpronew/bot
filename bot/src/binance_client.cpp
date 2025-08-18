#include "binance_client.hpp"

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// callback for curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t real_size = size * nmemb;
    string* mem = static_cast<string*>(userp);
    mem->append(static_cast<char*>(contents), real_size);
    return real_size;
}

long long BinanceClient::current_timestamp_ms()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

string BinanceClient::hmac_sha256(const string& key, const string& data)
{
    unsigned int len = 32;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned char* res = HMAC(EVP_sha256(),
                             key.c_str(), (int)key.length(),
                             (const unsigned char*)data.c_str(), data.length(),
                             digest, &len);
    if (!res) return "";
    ostringstream oss;
    oss << hex << setfill('0');
    for (unsigned int i = 0; i < len; ++i) {
        oss << setw(2) << (int)digest[i];
    }
    return oss.str();
}

string BinanceClient::sign_request(const string& query_string)
{
    return hmac_sha256(secret_key_, query_string);
}

BinanceClient::BinanceClient(const string& api_key, const string& secret_key, bool sandbox)
    : api_key_(api_key), secret_key_(secret_key), sandbox_(sandbox)
{
    base_url_ = sandbox_ ? "https://testnet.binance.vision/api" : "https://api.binance.com/api";
    cout << "[DEBUG] BinanceClient constructed. api_key.len=" << api_key_.size()
         << " secret_key.len=" << secret_key_.size() << " base_url=" << base_url_ << endl;
}

double BinanceClient::get_price(const string& symbol)
{
    string url = base_url_ + "/v3/ticker/price?symbol=" + symbol;

    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to init curl" << endl;
        return -1;
    }

    string read_buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        return -1;
    }

    try {
        auto json_resp = json::parse(read_buffer);
        string price_str = json_resp.at("price").get<string>();
        return stod(price_str);
    } catch (const exception& e) {
        cerr << "JSON parse error: " << e.what() << "\nResponse: " << read_buffer << endl;
        return -1;
    }
}

string BinanceClient::place_order(const string& symbol,
                                  const string& side,
                                  const string& type,
                                  double price,
                                  double quantity,
                                  const string& timeInForce)
{
    try {
        cout << "[place_order] start: symbol=" << symbol
             << " side=" << side << " type=" << type
             << " price=" << price << " qty=" << quantity << endl;

        long long ts = current_timestamp_ms();

        ostringstream qs;
        qs << "symbol=" << symbol
           << "&side=" << side
           << "&type=" << type
           << "&timestamp=" << ts
           << "&quantity=" << fixed << setprecision(8) << quantity;

        if (type == "LIMIT") {
            qs << "&timeInForce=" << timeInForce
               << "&price=" << fixed << setprecision(8) << price;
        }

        string query = qs.str();
        cout << "[place_order] query built, len=" << query.size() << endl;

        string sig = sign_request(query);
        cout << "[place_order] signature len=" << sig.size() << endl;

        string post_fields = query + "&signature=" + sig;
        string url = base_url_ + "/v3/order";
        cout << "[place_order] url=" << url << " post_fields_len=" << post_fields.size() << endl;

        CURL* curl = curl_easy_init();
        if (!curl) {
            cerr << "[place_order] curl_easy_init failed" << endl;
            return R"({"error":"curl_init_failed"})";
        }

        string read_buffer;

        string api_header = string("X-MBX-APIKEY: ") + api_key_;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(nullptr, api_header.c_str());
        if (!headers) {
            cerr << "[place_order] curl_slist_append returned NULL" << endl;
            curl_easy_cleanup(curl);
            return R"({"error":"curl_slist_append_failed"})";
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            ostringstream ers;
            ers << R"({"error":"curl_error","msg":")" << curl_easy_strerror(res) << "\"}";
            cerr << "[place_order] curl error: " << curl_easy_strerror(res) << endl;
            return ers.str();
        }

        cout << "[place_order] got response len=" << read_buffer.size() << endl;
        cout << "[place_order] response: " << read_buffer << endl;

        return read_buffer;
    } catch (const bad_alloc& e) {
        cerr << "[place_order] Caught bad_alloc: " << e.what() << endl;
        ostringstream ers; ers << R"({"error":"bad_alloc","msg":")" << e.what() << "\"}"; return ers.str();
    } catch (const exception& e) {
        cerr << "[place_order] Caught exception: " << e.what() << endl;
        ostringstream ers; ers << R"({"error":"exception","msg":")" << e.what() << "\"}"; return ers.str();
    } catch (...) {
        cerr << "[place_order] Caught unknown exception" << endl;
        return R"({"error":"unknown_exception"})";
    }
}

void BinanceClient::place_limit_order(const string& symbol,
                                      const string& side,
                                      double price,
                                      double quantity)
{
    string resp = place_order(symbol, side, "LIMIT", price, quantity, "GTC");
    (void)resp;
}

string BinanceClient::get_open_orders(const string& symbol)
{
    long long ts = current_timestamp_ms();
    ostringstream qs;
    qs << "symbol=" << symbol << "&timestamp=" << ts;
    string query = qs.str();
    string sig = sign_request(query);
    string url = base_url_ + "/v3/openOrders?" + query + "&signature=" + sig;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(nullptr, ("X-MBX-APIKEY: " + api_key_).c_str());

    string response;
    CURL* curl = curl_easy_init();
    if (!curl) return "{}";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        cerr << "curl error: " << curl_easy_strerror(res) << endl;
        return "{}";
    }

    cout << "Open orders: " << response << endl;
    return response;
}

void BinanceClient::print_compact_orders(const string& symbol, int64_t filter_order_id)
{
    string resp = get_open_orders(symbol);
    try {
        auto j = json::parse(resp);
        if (!j.is_array()) {
            cout << "Open orders (raw): " << resp << endl;
            return;
        }
        cout << "Open orders (compact):" << endl;
        for (auto &o : j) {
            long long oid = o.value("orderId", 0LL);
            if (filter_order_id != 0 && oid <= filter_order_id) continue;
            string side = o.value("side", "");
            string price = o.value("price", "");
            string qty = o.value("origQty", "");
            string status = o.value("status", "");
            cout << "  id=" << oid << " " << side << " price=" << price << " qty=" << qty << " status=" << status << endl;
        }
    } catch (const exception& e) {
        cout << "Failed to parse open orders: " << e.what() << "\nRaw: " << resp << endl;
    }
}

string BinanceClient::get_order_history(const string& symbol)
{
    long long ts = current_timestamp_ms();
    ostringstream qs;
    qs << "symbol=" << symbol << "&timestamp=" << ts;
    string query = qs.str();
    string sig = sign_request(query);
    string url = base_url_ + "/v3/allOrders?" + query + "&signature=" + sig;

    string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(nullptr, ("X-MBX-APIKEY: " + api_key_).c_str());

    CURL* curl = curl_easy_init();
    if (!curl) return "[]";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        cerr << "curl error: " << curl_easy_strerror(res) << endl;
        return "[]";
    }

    return response;
}
