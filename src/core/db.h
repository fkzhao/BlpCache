//
// Created by fakzhao on 2025/7/31.
//
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <leveldb/db.h>

#include "common/aof.h"

namespace blp {
    class LevelDBWrapper {
    public:

        // initialize the database with the specified path and options
        [[nodiscard]] bool init(const std::string& dbPath, size_t cacheSizeMB = 128, size_t writeBufferSizeMB = 64, int maxOpenFiles = 1000) const;

        // put a key-value pair into the database
        [[nodiscard]] bool put(const std::string& key, const std::string& value) const;

        // get a value by key from the database
        bool get(const std::string& key, std::string* value) const;

        // check if a key exists in the database
        [[nodiscard]] bool remove(const std::string& key) const;

        // check if a key exists in the database
        bool writeBatch(leveldb::WriteBatch &batch) const;

        //
        [[nodiscard]] int64_t ttl(const std::string& key) const;

        //
        [[nodiscard]] bool expire(const std::string& key, const uint64_t ts) const;

        // get the underlying leveldb database instance
        [[nodiscard]] const leveldb::Snapshot* createSnapshot() const;

        // release a snapshot created by createSnapshot
        // Note: The snapshot must be released after use to avoid memory leaks
        void releaseSnapshot(const leveldb::Snapshot* snapshot) const;

        explicit LevelDBWrapper(aof::Aof &aof);

        ~LevelDBWrapper();

    private:

        class Impl;
        std::unique_ptr<Impl> impl_;
        aof::Aof &aof_;
        void ttl_cleaner() const;
    };

    LevelDBWrapper* init_db(const std::string &db_path, aof::Aof &aof);
}


