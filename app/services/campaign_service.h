#pragma once
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

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

    std::optional<models::Campaign> getCampaignStats(int64_t id) {
        return fetchCampaign(id);
    }

    models::StatsSummary getSummaryStats() {
        std::unordered_map<std::string, models::PromoStats> by_name;
        std::vector<std::string> order;
        models::StatsSummary summary;

        db_.query(
            "SELECT name, phone, status FROM campaigns "
            "ORDER BY id ASC",
            {},
            [&](sqlite3_stmt* stmt) {
                std::string name = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 0));
                std::string phone = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 1));
                std::string status = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 2));

                auto it = by_name.find(name);
                if (it == by_name.end()) {
                    models::PromoStats ps;
                    ps.name = name;
                    by_name.emplace(name, ps);
                    order.push_back(name);
                    it = by_name.find(name);
                }
                models::PromoStats& ps = it->second;
                ps.total += 1;
                summary.total += 1;

                if (status == "sent") {
                    ps.sent += 1;
                    ps.sent_phones.push_back(phone);
                    summary.sent += 1;
                } else if (status == "skipped") {
                    ps.skipped += 1;
                    ps.skipped_phones.push_back(phone);
                    summary.skipped += 1;
                } else if (status == "failed") {
                    ps.failed += 1;
                    summary.failed += 1;
                } else {
                    ps.created += 1;
                    summary.created += 1;
                }
            });

        for (const auto& name : order) {
            summary.promos.push_back(by_name[name]);
        }
        return summary;
    }

    void printSummaryStats(std::ostream& out) {
        models::StatsSummary s = getSummaryStats();
        out << "\n=== Сводная статистика рассылок ===\n";
        if (s.promos.empty()) {
            out << "Кампаний пока нет.\n";
            return;
        }
        for (const auto& p : s.promos) {
            out << "\nАкция: \"" << p.name << "\"\n";
            out << "  Всего кампаний: " << p.total << "\n";
            out << "  Отправлено:     " << p.sent << "\n";
            if (!p.sent_phones.empty()) {
                out << "    -> ";
                for (size_t i = 0; i < p.sent_phones.size(); ++i) {
                    if (i) out << ", ";
                    out << p.sent_phones[i];
                }
                out << "\n";
            }
            out << "  Пропущено (отписаны): " << p.skipped << "\n";
            if (!p.skipped_phones.empty()) {
                out << "    -> ";
                for (size_t i = 0; i < p.skipped_phones.size(); ++i) {
                    if (i) out << ", ";
                    out << p.skipped_phones[i];
                }
                out << "\n";
            }
            out << "  Ошибка:         " << p.failed << "\n";
            out << "  В очереди:      " << p.created << "\n";
        }
        out << "\n=== ИТОГО ===\n";
        out << "Всего кампаний:       " << s.total << "\n";
        out << "Отправлено:           " << s.sent << "\n";
        out << "Пропущено (отписаны): " << s.skipped << "\n";
        out << "Ошибка:               " << s.failed << "\n";
        out << "В очереди:            " << s.created << "\n";
        out.flush();
    }

    bool resendCampaign(int64_t id) {
        std::optional<models::Campaign> campaign = fetchCampaign(id);
        if (!campaign) {
            std::cerr << "[ERROR] Campaign not found: " << id << "\n";
            return false;
        }

        if (campaign->status != "failed") {
            std::cerr << "[ERROR] Campaign " << id
                    << " has status '" << campaign->status
                    << "', resend only allowed for 'failed'\n";
            return false;
        }

        if (isUnsubscribed(campaign->phone)) {
            std::cout << "[INFO] Phone " << campaign->phone
                    << " is unsubscribed, skipping resend\n";
            updateStatus(id, "skipped");
            return false;
        }

        std::string msg = personalize(campaign->message_template,
                                    campaign->recipient_name);
        bool sent = sms_.send(campaign->phone, msg);
        updateStatus(id, sent ? "sent" : "failed");
        return sent;
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
