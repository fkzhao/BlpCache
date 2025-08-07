//
// Created by fakzhao on 2025/7/31.
//

#include "config.h"

#include <iostream>
#include <sstream>
#include <cerrno> // IWYU pragma: keep
#include <fstream> // IWYU pragma: keep
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace blp::config {
    DEFINE_Int16(port, "6479");
    DEFINE_Int16(replica_port, "6480");
    DEFINE_String(replicate_host, "127.0.0.1");
    DEFINE_String(model, "master");

    std::map<std::string, Register::Field> *Register::_s_field_map = nullptr;

    std::mutex custom_conf_lock;
    std::mutex mutable_string_config_lock;

    std::string trim(const std::string &str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }

    template<typename T>
    std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
        size_t last = v.size() - 1;
        for (size_t i = 0; i < v.size(); ++i) {
            out << v[i];
            if (i != last) {
                out << ", ";
            }
        }
        return out;
    }


bool init(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open config file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        // Check if key exists in field map
        if (Register::_s_field_map && Register::_s_field_map->find(key) != Register::_s_field_map->end()) {
            Register::Field& field = (*Register::_s_field_map)[key];
            field.set_value(value);
        }
    }
    file.close();
    return true;
}

}
