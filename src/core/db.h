//
// Created by fakzhao on 2025/7/31.
//
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <leveldb/db.h>

namespace blp {
    class LevelDBWrapper {
    public:

        // get singleton instance
        static LevelDBWrapper* getInstance();

        // initialize the database with the specified path and options
        bool init(const std::string& dbPath, size_t cacheSizeMB = 128, size_t writeBufferSizeMB = 64, int maxOpenFiles = 1000) const;

        // put a key-value pair into the database
        bool put(const std::string& key, const std::string& value) const;

        // get a value by key from the database
        bool get(const std::string& key, std::string* value) const;

        // check if a key exists in the database
        bool remove(const std::string& key) const;

        // check if a key exists in the database
        bool writeBatch(leveldb::WriteBatch &batch) const;

        // get the underlying leveldb database instance
        [[nodiscard]] const leveldb::Snapshot* createSnapshot() const;

        // release a snapshot created by createSnapshot
        // Note: The snapshot must be released after use to avoid memory leaks
        void releaseSnapshot(const leveldb::Snapshot* snapshot) const;

        LevelDBWrapper(const LevelDBWrapper&) = delete;
        LevelDBWrapper& operator=(const LevelDBWrapper&) = delete;

        ~LevelDBWrapper();

    private:
        LevelDBWrapper();

        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    bool init_db();
}


