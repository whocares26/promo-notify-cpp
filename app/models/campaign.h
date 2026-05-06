#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace models {

    struct Campaign {
        int64_t id = 0;
        std::string name;
        std::string phone;
        std::string message_template;
        std::string recipient_name;
        std::string status;
    };

    struct PromoStats {
        std::string name;
        int total = 0;
        int sent = 0;
        int skipped = 0;
        int failed = 0;
        int created = 0;
        std::vector<std::string> sent_phones;
        std::vector<std::string> skipped_phones;
    };

    struct StatsSummary {
        std::vector<PromoStats> promos;
        int total = 0;
        int sent = 0;
        int skipped = 0;
        int failed = 0;
        int created = 0;
    };

}  // namespace models
