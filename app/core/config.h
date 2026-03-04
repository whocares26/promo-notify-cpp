#pragma once
#include <string>
#include <boost/program_options.hpp>

namespace config {

struct AppConfig {
    int port = 18080;
    std::string sms_email = "";
    std::string sms_api_key = "";
    std::string sms_sign = "SMS Aero";
    std::string sms_phone = "";
    bool sms_ru_use_mock = false;

    static AppConfig load() {
        AppConfig cfg;

        namespace po = boost::program_options;

        po::options_description desc;
        desc.add_options()
            ("APP_PORT",        po::value<int>()->default_value(18080))
            ("SMS_EMAIL",       po::value<std::string>()->default_value(""))
            ("SMS_API_KEY",     po::value<std::string>()->default_value(""))
            ("SMS_SIGN",        po::value<std::string>()->default_value("SMS Aero"))
            ("SMS_PHONE",       po::value<std::string>()->default_value(""))
            ("SMS_RU_USE_MOCK", po::value<bool>()->default_value(false))
        ;

        po::variables_map vm;

        auto filter = [](const std::string& name) -> std::string {
            if (name == "APP_PORT"     ||
                name == "SMS_EMAIL"    ||
                name == "SMS_API_KEY"  ||
                name == "SMS_SIGN"     ||
                name == "SMS_PHONE"    ||
                name == "SMS_RU_USE_MOCK") {
                return name;
            }
            return std::string();
        };

        po::store(po::parse_environment(desc, filter), vm);
        po::notify(vm);

        cfg.port            = vm["APP_PORT"].as<int>();
        cfg.sms_email       = vm["SMS_EMAIL"].as<std::string>();
        cfg.sms_api_key     = vm["SMS_API_KEY"].as<std::string>();
        cfg.sms_sign        = vm["SMS_SIGN"].as<std::string>();
        cfg.sms_phone       = vm["SMS_PHONE"].as<std::string>();
        cfg.sms_ru_use_mock = vm["SMS_RU_USE_MOCK"].as<bool>();

        return cfg;
    }
};

} // namespace config