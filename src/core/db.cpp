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

    LevelDBWrapper::LevelDBWrapper(aof::Aof &aof) : impl_(std::make_unique<Impl>()), aof_(aof) {
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
        return impl_->put(key, value);
    }

    bool LevelDBWrapper::get(const std::string &key, std::string *value) const {
        return impl_->get(key, value);
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
