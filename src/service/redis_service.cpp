//
// Created by fakzhao on 2025/8/1.
//

#include "redis_service.h"
#include "core/db.h"

namespace blp {
    RedisServiceImpl::RedisServiceImpl(LevelDBWrapper *db) {
        db_ = db;
        if (!db_) {
            LOG(ERROR) << "Failed to initialize RedisServiceImpl: db is null";
            throw std::runtime_error("Database not initialized");
        }
    }

    bool RedisServiceImpl::Set(const std::string &key, const std::string &value) const {
        if (!db_->put(key, value)) {
            LOG(ERROR) << "Failed to set key in storage";
            return false;
        }
        return true;
    }

    bool RedisServiceImpl::Auth(const std::string &password) {
        if (password != "123456") {
            LOG(ERROR) << "Invalid password";
            return false;
        }
        return true;
    }

    bool RedisServiceImpl::Get(const std::string &key, std::string *value) const {
        return db_->get(key, value);
    }

    bool RedisServiceImpl::Del(const std::string &key) const {
        if (! db_->remove(key)) {
            LOG(ERROR) << "Failed to delete key in storage";
            return false;
        }
        return true;
    }

    int64_t RedisServiceImpl::Ttl(const std::string &key) const {
        return db_->ttl(key);
    }

    bool RedisServiceImpl::Expire(const std::string &key, uint64_t ts) const {
        return db_->expire(key, ts); // Set expiration to 0 means delete the key
    }

    void RedisServiceImpl::AddCommandHandler(const std::string &command, std::unique_ptr<brpc::RedisCommandHandler> handler) {
        command_handlers_[command] = std::move(handler);
        RedisService::AddCommandHandler(command, command_handlers_[command].get());
    }

}
