#pragma once
#include <iostream>
#include <optional>
#include <string>

#include "core/database.h"
#include "models/campaign.h"
#include "services/sms_aero_client.h"

namespace services {

class CampaignService {
 public:
    CampaignService(core::Database& db, SmsAeroClient& sms)
        : db_(db), sms_(sms) {}

    int64_t createCampaign(const models::Campaign& c) {
        return db_.insert(
            "INSERT INTO campaigns "
            "(name, phone, message_template, recipient_name, status) "
            "VALUES (?, ?, ?, ?, 'created')",
            {c.name, c.phone, c.message_template, c.recipient_name});
    }

    bool sendCampaign(int64_t id) {
        std::optional<models::Campaign> campaign = fetchCampaign(id);
        if (!campaign) {
            std::cerr << "[ERROR] Campaign not found: " << id << "\n";
            return false;
        }

        if (isUnsubscribed(campaign->phone)) {
            std::cout << "[INFO] Phone " << campaign->phone
                      << " is unsubscribed, skipping\n";
            updateStatus(id, "skipped");
            return false;
        }

        std::string msg = personalize(campaign->message_template,
                                      campaign->recipient_name);
        bool sent = sms_.send(campaign->phone, msg);
        updateStatus(id, sent ? "sent" : "failed");
        return sent;
    }

    void unsubscribe(const std::string& phone) {
        db_.insert(
            "INSERT OR IGNORE INTO unsubscribed (phone) VALUES (?)",
            {phone});
        std::cout << "[INFO] Unsubscribed: " << phone << "\n";
    }

 private:
    core::Database& db_;
    SmsAeroClient& sms_;

    std::optional<models::Campaign> fetchCampaign(int64_t id) {
        std::optional<models::Campaign> result;
        db_.query(
            "SELECT id, name, phone, message_template, recipient_name, status "
            "FROM campaigns WHERE id = ?",
            {std::to_string(id)},
            [&result](sqlite3_stmt* stmt) {
                models::Campaign c;
                c.id   = sqlite3_column_int64(stmt, 0);
                c.name = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 1));
                c.phone = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 2));
                c.message_template = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 3));
                c.recipient_name = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 4));
                c.status = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 5));
                result = c;
            });
        return result;
    }

    bool isUnsubscribed(const std::string& phone) {
        bool found = false;
        db_.query(
            "SELECT 1 FROM unsubscribed WHERE phone = ?",
            {phone},
            [&found](sqlite3_stmt*) { found = true; });
        return found;
    }

    void updateStatus(int64_t id, const std::string& status) {
        db_.insert(
            "UPDATE campaigns SET status = ? WHERE id = ?",
            {status, std::to_string(id)});
    }

    static std::string personalize(const std::string& tmpl,
                                   const std::string& name) {
        std::string result = tmpl;
        const std::string placeholder = "{name}";
        auto pos = result.find(placeholder);
        if (pos != std::string::npos) {
            result.replace(pos, placeholder.size(), name);
        }
        return result;
    }
};

}  // namespace services
