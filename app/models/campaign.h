#pragma once
#include <cstdint>
#include <string>

namespace models {

    struct Campaign {
        int64_t id = 0;
        std::string name;
        std::string phone;
        std::string message_template;
        std::string recipient_name;
        std::string status;
    };

}  // namespace models
