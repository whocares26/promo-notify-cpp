#pragma once

#include <string>
#include <boost/program_options.hpp>

namespace config {

struct AppConfig {
    int port = 18080;
    std::string db_path = "/data/promo_notify.db";
    std::string sms_ru_api_key = "";
    bool sms_ru_use_mock = false;
    std::string log_level = "info";

    static AppConfig load() {
        AppConfig cfg;
        
        namespace po = boost::program_options;
        
        po::options_description desc;
        desc.add_options()
            ("APP_PORT", po::value<int>()->default_value(18080))
            ("DB_PATH", po::value<std::string>()->default_value("/data/promo_notify.db"))
            ("SMS_RU_API_KEY", po::value<std::string>()->default_value(""))
            ("SMS_RU_USE_MOCK", po::value<bool>()->default_value(false))
            ("LOG_LEVEL", po::value<std::string>()->default_value("info"))
        ;
        
        po::variables_map vm;
        
        auto filter = [&desc](const std::string& name) -> std::string {
            if (desc.find_nothrow(name, false)) {
                return name;
            }
            return std::string();
        };
        
        po::store(po::parse_environment(desc, filter), vm);
        po::notify(vm);
        
        cfg.port = vm["APP_PORT"].as<int>();
        cfg.db_path = vm["DB_PATH"].as<std::string>();
        cfg.sms_ru_api_key = vm["SMS_RU_API_KEY"].as<std::string>();
        cfg.sms_ru_use_mock = vm["SMS_RU_USE_MOCK"].as<bool>();
        cfg.log_level = vm["LOG_LEVEL"].as<std::string>();
        
        return cfg;
    }
};

} // namespace config