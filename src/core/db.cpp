//
// Created by fakzhao on 2025/7/31.
//

#include "db.h"
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include <mutex>
#include <string>
#include <iostream>

namespace blp {
    class LevelDBWrapper::Impl {
    public:
        Impl() : db_(nullptr), cache_(nullptr), filter_policy_(nullptr) {
        }

        bool init(const std::string &dbPath, const size_t cacheSizeMB, const size_t writeBufferSizeMB, const int maxOpenFiles) {
            leveldb::Options options;
            options.create_if_missing = true;
            options.error_if_exists = false;

            // Cache and performance options
            options.block_cache = cache_ = leveldb::NewLRUCache(cacheSizeMB * 1024 * 1024);
            options.write_buffer_size = writeBufferSizeMB * 1024 * 1024;
            options.max_open_files = maxOpenFiles;
            options.filter_policy = filter_policy_ = leveldb::NewBloomFilterPolicy(10);

            leveldb::Status status = leveldb::DB::Open(options, dbPath, &db_);
            if (!status.ok()) {
                std::cerr << "Failed to open LevelDB at " << dbPath << ": " << status.ToString() << std::endl;
                return false;
            }
            return true;
        }

        bool put(const std::string &key, const std::string &value) const {
            if (!db_) return false;
            std::lock_guard<std::mutex> lock(mutex_);
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            leveldb::Status status = db_->Put(writeOptions, key, value);
            return status.ok();
        }

        bool get(const std::string &key, std::string *value) const {
            if (!db_) return false;
            leveldb::ReadOptions readOptions;
            leveldb::Status status = db_->Get(readOptions, key, value);
            return status.ok();
        }

        bool remove(const std::string &key) const {
            if (!db_) return false;
            std::lock_guard<std::mutex> lock(mutex_);
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            leveldb::Status status = db_->Delete(writeOptions, key);
            return status.ok();
        }

        bool writeBatch(leveldb::WriteBatch &batch) const {
            if (!db_) return false;
            std::lock_guard<std::mutex> lock(mutex_);
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            const leveldb::Status status = db_->Write(writeOptions, &batch);
            return status.ok();
        }

        const leveldb::Snapshot *createSnapshot() const {
            if (!db_) return nullptr;
            return db_->GetSnapshot();
        }

        void releaseSnapshot(const leveldb::Snapshot *snapshot) const {
            if (db_ && snapshot) {
                db_->ReleaseSnapshot(snapshot);
            }
        }

        leveldb::Iterator* newIterator() {
            leveldb::ReadOptions readOptions;
            return db_ ? db_->NewIterator(readOptions) : nullptr;
        }

        ~Impl() {
            delete db_;
            delete cache_;
            delete filter_policy_;
        }

    private:
        leveldb::DB *db_;
        leveldb::Cache *cache_;
        const leveldb::FilterPolicy *filter_policy_;
        mutable std::mutex mutex_;
    };

    uint64_t now_sec() {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count());
    }



    LevelDBWrapper::LevelDBWrapper(aof::Aof &aof) : impl_(std::make_unique<Impl>()), aof_(aof) {
        std::thread cleaner(&LevelDBWrapper::ttl_cleaner, this);
        cleaner.detach();
    }


    void LevelDBWrapper::ttl_cleaner() const {
        while (true) {
            leveldb::Iterator* it = impl_->newIterator();
            if (!it) {
                continue;
            }
            uint64_t now = now_sec();
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                std::string key = it->key().ToString();
                auto* value = new std::string(it->value().ToString());
                size_t tab_pos = value->find('\t');
                if (tab_pos != std::string::npos) {
                    const std::string expire_time_str = value->substr(tab_pos + 1);
                    try {
                        const uint64_t expire_time = std::stoull(expire_time_str);
                        *value = value->substr(0, tab_pos);
                        if (expire_time == -1) {
                            continue;
                        }
                        uint64_t current_time = now_sec();
                        if (expire_time <= current_time) {
                            bool res = impl_->remove(key);
                            if (!res) {
                                std::cerr << "Failed to remove expired key: " << key << std::endl;
                            } else {
                                std::cout << "Removed expired key: " << key << std::endl;
                            }
                        }
                    } catch (const std::exception&) {
                        continue;
                    }
                }
            }
            delete it;
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }

    LevelDBWrapper::~LevelDBWrapper() = default;

    bool LevelDBWrapper::init(const std::string &dbPath, size_t cacheSizeMB, size_t writeBufferSizeMB, int maxOpenFiles) const {
        return impl_->init(dbPath, cacheSizeMB, writeBufferSizeMB, maxOpenFiles);
    }

    bool LevelDBWrapper::put(const std::string &key, const std::string &value) const {
        std::vector<std::string> cmd = {"SET", key, value};
        if (!aof_.appendCommandAsync(cmd)) {
            aof_.appendCommandBlocking(cmd);
        }
        const std::string stored_value = value + "\t-1";
        return impl_->put(key, stored_value);
    }

    bool LevelDBWrapper::get(const std::string &key, std::string *value) const {
        impl_->get(key, value);
        if (value->empty()) {
            return false; // Key does not exist
        }
        size_t tab_pos = value->find('\t');
        if (tab_pos != std::string::npos) {
            const std::string expire_time_str = value->substr(tab_pos + 1);
            try {
                const uint64_t expire_time = std::stoull(expire_time_str);
                *value = value->substr(0, tab_pos);
                if (expire_time == -1) {
                    return true;
                }
                uint64_t current_time = now_sec();
                if (expire_time <= current_time) {
                    bool res = impl_->remove(key); // Key has expired, remove it
                    if (!res) {
                        std::cerr << "Failed to remove expired key: " << key << std::endl;
                    }
                    value->clear(); // Clear the value to indicate it does not exist
                    return false;
                }
            } catch (const std::exception&) {
                return false;
            }
        }
        return true;
    }

    bool LevelDBWrapper::remove(const std::string &key) const {
        std::vector<std::string> cmd = {"DEL", key};
        if (!aof_.appendCommandAsync(cmd)) {
            aof_.appendCommandBlocking(cmd);
        }
        return impl_->remove(key);
    }

    bool LevelDBWrapper::writeBatch(leveldb::WriteBatch &batch) const {
        return impl_->writeBatch(batch);
    }

    const leveldb::Snapshot *LevelDBWrapper::createSnapshot() const {
        return impl_->createSnapshot();
    }

    void LevelDBWrapper::releaseSnapshot(const leveldb::Snapshot *snapshot) const {
        impl_->releaseSnapshot(snapshot);
    }

    int64_t LevelDBWrapper::ttl(const std::string& key) const {
        std::string value;
        impl_->get(key, &value);
        if (value.empty()) {
            return -2; // Key does not exist
        }
        size_t tab_pos = value.find('\t');
        if (tab_pos == std::string::npos) {
            return -2; // No expiration time set
        }

        const std::string expire_time_str = value.substr(tab_pos + 1);
        try {
            const uint64_t expire_time = std::stoull(expire_time_str);
            if (expire_time == -1) {
                return -1; // No expiration time set
            }
            const uint64_t current_time = now_sec();
            if (expire_time <= current_time) {
                return -2;
            }

            return expire_time - current_time;
        } catch (const std::exception&) {
            return -1;
        }
    }

    bool LevelDBWrapper::expire(const std::string& key, const uint64_t ts) const {
        std::string value;
        impl_->get(key, &value);
        if (value.empty()) {
            return false; // Key does not exist
        }
        size_t tab_pos = value.find('\t');
        std::string actual_value;
        if (tab_pos == std::string::npos) {
            actual_value = value;
        } else {
            actual_value = value.substr(0, tab_pos);
        }
        const uint64_t expire_time = now_sec() + ts;
        const std::string new_value = actual_value + "\t" + std::to_string(expire_time);
        return put(key, new_value);
    }


    LevelDBWrapper* init_db(const std::string &db_path, blp::aof::Aof &aof) {
        // Initialize LevelDB storage
        auto *storage = new LevelDBWrapper(aof);
        if (!storage->init(db_path, 256, 128, 2000)) {
            std::cerr << "Failed to initialize LevelDB" << std::endl;
            return nullptr;
        }
        return storage;
    }
}
