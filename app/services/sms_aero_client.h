#pragma once
#include <string>
#include <iostream>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

namespace services {

class SmsAeroClient {
public:
    SmsAeroClient(const std::string& email,
                  const std::string& api_key,
                  const std::string& sign = "SMS Aero",
                  bool use_mock = false)
        : email_(email), api_key_(api_key), sign_(sign), use_mock_(use_mock) {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~SmsAeroClient() {
        curl_global_cleanup();
    }

    bool send(const std::string& phone, const std::string& message) {
        if (use_mock_) {
            std::cout << "[MOCK SMS] To: " << phone << ", Message: " << message << "\n";
            return true;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[ERROR] Failed to init CURL\n";
            return false;
        }

        nlohmann::json body;
        body["number"]  = phone;
        body["text"]    = message;
        body["sign"]    = sign_;
        body["channel"] = "SERVICE";
        std::string body_str = body.dump();

        std::string user_pwd = email_ + ":" + api_key_;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://gate.smsaero.ru/v2/sms/send");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_str.size());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERPWD, user_pwd.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "[ERROR] CURL: " << curl_easy_strerror(res) << "\n";
            return false;
        }
        if (http_code != 200) {
            std::cerr << "[ERROR] HTTP " << http_code << ": " << response << "\n";
            return false;
        }

        std::cout << "[INFO] SmsAero response: " << response << "\n";
        return parseResponse(response);
    }

private:
    std::string email_;
    std::string api_key_;
    std::string sign_;
    bool use_mock_;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    bool parseResponse(const std::string& response) {
        try {
            auto json = nlohmann::json::parse(response);
            if (json.value("success", false)) {
                std::cout << "[INFO] SMS sent successfully\n";
                return true;
            } else {
                std::cerr << "[ERROR] SmsAero: " << json.value("message", "unknown error") << "\n";
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to parse response: " << e.what() << " - " << response << "\n";
            return false;
        }
    }
};

} // namespace services