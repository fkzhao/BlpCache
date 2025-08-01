//
// Created by fakzhao on 2025/8/1.
//

#pragma once

#include <brpc/redis.h>

namespace blp {
    class AuthSession : public brpc::Destroyable {
    public:
        explicit AuthSession(const std::string &password)
            : _password(password) {
        }

        void Destroy() override {
            delete this;
        }

        const std::string _password;
    };
}
