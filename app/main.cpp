#include "crow.h"
#include "core/config.h"
#include "services/sms_aero_client.h"

int main() {
    config::AppConfig cfg = config::AppConfig::load();

    services::SmsAeroClient sms_client(
        cfg.sms_email,
        cfg.sms_api_key,
        cfg.sms_sign,
        cfg.sms_ru_use_mock
    );

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([&sms_client, &cfg]() {
        crow::json::wvalue response;

        if (cfg.sms_phone.empty() || cfg.sms_email.empty() || cfg.sms_api_key.empty()) {
            response["status"] = "error";
            response["message"] = "SMS_PHONE, SMS_EMAIL or SMS_API_KEY not configured";
            return response;
        }

        bool sent = sms_client.send(cfg.sms_phone, "Тест от promo-notify");

        if (sent) {
            response["status"] = "ok";
            response["sms_sent"] = true;
            response["phone"] = cfg.sms_phone;
        } else {
            response["status"] = "error";
            response["sms_sent"] = false;
            response["message"] = "Failed to send SMS. Check logs.";
        }
        return response;
    });

    CROW_ROUTE(app, "/health")
    ([]() {
        crow::json::wvalue response;
        response["status"] = "ok";
        return response;
    });

    app.port(cfg.port)
       .multithreaded()
       .run();

    return 0;
}