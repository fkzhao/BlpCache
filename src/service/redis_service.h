//
// Created by fakzhao on 2025/8/1.
//
#pragma once
#include <string>
#include <brpc/redis.h>

#include "core/db.h"

namespace blp {
    class RedisServiceImpl final : public brpc::RedisService {
    public:
        explicit RedisServiceImpl(LevelDBWrapper *db);

        [[nodiscard]] bool Set(const std::string &key, const std::string &value) const;

        static bool Auth(const std::string &password);

        bool Get(const std::string &key, std::string *value) const;

        [[nodiscard]] bool Del(const std::string &key) const;

        [[nodiscard]] int64_t Ttl(const std::string &key) const;

        [[nodiscard]] bool Expire(const std::string &key, uint64_t ts) const;

        void AddCommandHandler(const std::string& command, std::unique_ptr<brpc::RedisCommandHandler> handler);

    private:
        LevelDBWrapper *db_;
        std::map<std::string, std::unique_ptr<brpc::RedisCommandHandler>> command_handlers_;
    };
}
