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
        RedisServiceImpl();

        [[nodiscard]] bool Set(const std::string &key, const std::string &value) const;

        static bool Auth(const std::string &password);

        bool Get(const std::string &key, std::string *value) const;

        [[nodiscard]] bool Del(const std::string &key) const;

        void AddCommandHandler(const std::string& command, std::unique_ptr<brpc::RedisCommandHandler> handler);

    private:
        LevelDBWrapper *_db = LevelDBWrapper::getInstance();
        std::map<std::string, std::unique_ptr<brpc::RedisCommandHandler>> command_handlers_;
    };
}
