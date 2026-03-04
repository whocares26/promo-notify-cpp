#include "crow.h"
#include "core/config.h"
#include <iostream>

int main() {
    config::AppConfig cfg = config::AppConfig::load();

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        crow::json::wvalue response;
        response["status"] = "ok";
        response["service"] = "promo-notify-cpp";
        return response;
    });
    
    app.port(cfg.port)
       .multithreaded()
       .run();

    return 0;
}