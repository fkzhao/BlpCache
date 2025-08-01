//
// Created by fakzhao on 2025/7/31.
//

#include "config.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <cerrno> // IWYU pragma: keep
#include <cstring>
#include <fstream> // IWYU pragma: keep
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <vector>


namespace blp::config {
    DEFINE_String(custom_config_dir, "${BLP_HOME}/conf");
    DEFINE_String(persistent_dir, "${BLP_HOME}/data");
    DEFINE_Int32(port, "6379");
    DEFINE_String(host, "0.0.0.0");
    DEFINE_Int32(replicate_port, "6380");

    std::map<std::string, Register::Field> *Register::_s_field_map = nullptr;
    std::map<std::string, std::function<bool()> > *RegisterConfValidator::_s_field_validator = nullptr;
    std::map<std::string, std::string> *full_conf_map = nullptr;

    std::mutex custom_conf_lock;

    // init conf fields
    bool init(const char *conf_file, bool fill_conf_map, bool must_exist, bool set_to_default) {
        std::lock_guard<std::mutex> lock(custom_conf_lock);
        std::ifstream file(conf_file);
        if (!file.is_open()) {
            throw std::runtime_error(std::string("Failed to open config file: ") + conf_file);
        }
        if (fill_conf_map && full_conf_map == nullptr) {
            full_conf_map = new std::map<std::string, std::string>();
        }
        std::string line;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            size_t pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // 去除键和值的首尾空白
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (!key.empty()) {
                (*full_conf_map)[key] = value;
            }

            for (const auto& it : *Register::_s_field_map) {
                std::string field = it.first;
                std::string type = it.second.type;
                std::cout << field << " " << type << std::endl;
            }
        }

        return true;
    }
}
