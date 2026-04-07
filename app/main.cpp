#include "crow.h"
#include "core/config.h"
#include "core/database.h"
#include "models/campaign.h"
#include "services/campaign_service.h"
#include "services/sms_aero_client.h"

static const char* kInitSql =
    "CREATE TABLE IF NOT EXISTS campaigns ("
    "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name             TEXT    NOT NULL,"
    "  phone            TEXT    NOT NULL,"
    "  message_template TEXT    NOT NULL,"
    "  recipient_name   TEXT    NOT NULL DEFAULT '',"
    "  status           TEXT    NOT NULL DEFAULT 'created',"
    "  created_at       DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS unsubscribed ("
    "  phone      TEXT PRIMARY KEY,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");";

int main() {
    config::AppConfig cfg = config::AppConfig::load();

    core::Database db(cfg.db_path);
    db.execute(kInitSql);

    services::SmsAeroClient sms_client(
        cfg.sms_email,
        cfg.sms_api_key,
        cfg.sms_sign,
        cfg.sms_ru_use_mock);

    services::CampaignService campaign_service(db, sms_client);

    crow::SimpleApp app;

    // GET /health — проверка работоспособности сервиса
    CROW_ROUTE(app, "/health")
    ([]() {
        crow::json::wvalue res;
        res["status"] = "ok";
        return crow::response(200, res);
    });

    // POST /campaigns — создать кампанию
    // Body: { "name", "phone", "message_template", "recipient_name"? }
    CROW_ROUTE(app, "/campaigns").methods("POST"_method)
    ([&campaign_service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::json::wvalue res;
            res["status"]  = "error";
            res["message"] = "Invalid JSON";
            return crow::response(400, res);
        }

        models::Campaign c;
        c.name             = body["name"].s();
        c.phone            = body["phone"].s();
        c.message_template = body["message_template"].s();
        c.recipient_name   = body.has("recipient_name")
                         ? std::string(body["recipient_name"].s())
                         : std::string("");

        if (c.name.empty() || c.phone.empty() || c.message_template.empty()) {
            crow::json::wvalue res;
            res["status"]  = "error";
            res["message"] = "name, phone and message_template are required";
            return crow::response(400, res);
        }

        try {
            int64_t id = campaign_service.createCampaign(c);
            crow::json::wvalue res;
            res["status"] = "ok";
            res["id"]     = id;
            return crow::response(201, res);
        } catch (const std::exception& e) {
            crow::json::wvalue res;
            res["status"]  = "error";
            res["message"] = e.what();
            return crow::response(500, res);
        }
    });

    // POST /campaigns/<id>/send — отправить SMS по кампании
    CROW_ROUTE(app, "/campaigns/<int>/send").methods("POST"_method)
    ([&campaign_service](int id) {
        try {
            bool sent = campaign_service.sendCampaign(static_cast<int64_t>(id));
            crow::json::wvalue res;
            if (sent) {
                res["status"] = "ok";
                res["sent"]   = true;
            } else {
                res["status"]  = "error";
                res["sent"]    = false;
                res["message"] = "Not found, unsubscribed, or SMS failed";
            }
            return crow::response(sent ? 200 : 400, res);
        } catch (const std::exception& e) {
            crow::json::wvalue res;
            res["status"]  = "error";
            res["message"] = e.what();
            return crow::response(500, res);
        }
    });

    // POST /unsubscribe — отписать номер от уведомлений
    // Body: { "phone" }
    CROW_ROUTE(app, "/unsubscribe").methods("POST"_method)
    ([&campaign_service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::json::wvalue res;
            res["status"]  = "error";
            res["message"] = "Invalid JSON";
            return crow::response(400, res);
        }

        std::string phone = body["phone"].s();
        if (phone.empty()) {
            crow::json::wvalue res;
            res["status"]  = "error";
            res["message"] = "phone is required";
            return crow::response(400, res);
        }

        campaign_service.unsubscribe(phone);
        crow::json::wvalue res;
        res["status"]  = "ok";
        res["message"] = "Unsubscribed successfully";
        return crow::response(200, res);
    });

    app.port(cfg.port)
       .multithreaded()
       .run();

    return 0;
}
