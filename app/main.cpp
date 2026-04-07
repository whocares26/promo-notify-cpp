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

    // GET /campaigns/<id>/stats — аналитика по кампании
    CROW_ROUTE(app, "/campaigns/<int>/stats").methods("GET"_method)
    ([&campaign_service](int id) {
        auto campaign = campaign_service.getCampaignStats(
            static_cast<int64_t>(id));
        crow::json::wvalue res;
        if (!campaign) {
            res["status"]  = "error";
            res["message"] = "Campaign not found";
            return crow::response(404, res);
        }
        res["status"]           = "ok";
        res["id"]               = campaign->id;
        res["name"]             = campaign->name;
        res["phone"]            = campaign->phone;
        res["message_template"] = campaign->message_template;
        res["recipient_name"]   = campaign->recipient_name;
        res["sms_status"]       = campaign->status;
        return crow::response(200, res);
    });

    // POST /campaigns/<id>/resend — повторная отправка упавшей кампании
    CROW_ROUTE(app, "/campaigns/<int>/resend").methods("POST"_method)
    ([&campaign_service](int id) {
        crow::json::wvalue res;
        try {
            bool sent = campaign_service.resendCampaign(
                static_cast<int64_t>(id));
            if (sent) {
                res["status"] = "ok";
                res["sent"]   = true;
            } else {
                res["status"]  = "error";
                res["sent"]    = false;
                res["message"] = "Resend failed: wrong status, "
                                "unsubscribed, or SMS error";
            }
            return crow::response(sent ? 200 : 400, res);
        } catch (const std::exception& e) {
            res["status"]  = "error";
            res["message"] = e.what();
            return crow::response(500, res);
        }
    });

    app.port(cfg.port)
       .multithreaded()
       .run();

    return 0;
}
