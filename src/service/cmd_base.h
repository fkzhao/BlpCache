//
// Created by fakzhao on 2025/8/1.
//
#pragma once

#include <brpc/redis.h>

#include "auth.h"
#include "redis_service.h"

namespace blp {
    class CommandHandler : public brpc::RedisCommandHandler {
    public:
        explicit CommandHandler(RedisServiceImpl *rsimpl) : _rsimpl(rsimpl) {
        }
    protected:
        RedisServiceImpl *_rsimpl;
    };
}
