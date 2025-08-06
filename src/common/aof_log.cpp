//
// Created by fakzhao on 2025/8/6.
//

#include "aof_log.h"

namespace blp {
    AOFLog::AOFLog(const std::string &filename) : filename_(filename) {
    }

    void AOFLog::append(uint64_t sequence, const std::string &key, const std::string &value) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream ofs(filename_, std::ios::binary | std::ios::app);

        uint32_t key_len = key.size();
        uint32_t value_len = value.size();

        ofs.write(reinterpret_cast<const char *>(&sequence), sizeof(sequence));
        ofs.write(reinterpret_cast<const char *>(&key_len), sizeof(key_len));
        ofs.write(reinterpret_cast<const char *>(&value_len), sizeof(value_len));
        ofs.write(key.data(), key_len);
        ofs.write(value.data(), value_len);
        ofs.close();
    }

    void AOFLog::readAll(std::vector<DataEntry> &entries) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ifstream ifs(filename_, std::ios::binary);
        if (!ifs) return;

        while (ifs.peek() != EOF) {
            uint64_t sequence;
            uint32_t key_len, value_len;
            ifs.read(reinterpret_cast<char *>(&sequence), sizeof(sequence));
            ifs.read(reinterpret_cast<char *>(&key_len), sizeof(key_len));
            ifs.read(reinterpret_cast<char *>(&value_len), sizeof(value_len));

            std::string key(key_len, '\0');
            std::string value(value_len, '\0');
            ifs.read(&key[0], key_len);
            ifs.read(&value[0], value_len);

            DataEntry entry;
            entry.set_sequence(sequence);
            entry.set_key(key);
            entry.set_value(value);
            entries.push_back(entry);
        }
    }

    void AOFLog::readIncrements(uint64_t from_seq, std::vector<DataEntry> &entries) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ifstream ifs(filename_, std::ios::binary);
        if (!ifs) return;

        while (ifs.peek() != EOF) {
            uint64_t sequence;
            uint32_t key_len, value_len;
            ifs.read(reinterpret_cast<char *>(&sequence), sizeof(sequence));
            ifs.read(reinterpret_cast<char *>(&key_len), sizeof(key_len));
            ifs.read(reinterpret_cast<char *>(&value_len), sizeof(value_len));

            std::string key(key_len, '\0');
            std::string value(value_len, '\0');
            ifs.read(&key[0], key_len);
            ifs.read(&value[0], value_len);

            if (sequence > from_seq) {
                DataEntry entry;
                entry.set_sequence(sequence);
                entry.set_key(key);
                entry.set_value(value);
                entries.push_back(entry);
            }
        }
    }
}
