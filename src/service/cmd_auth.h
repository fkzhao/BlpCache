//
// Created by fakzhao on 2025/8/1.
//

#pragma once

#include <brpc/redis.h>

#include "cmd_base.h"


namespace blp {
    class AuthCommandHandler final : public CommandHandler {
    public:
        explicit AuthCommandHandler(RedisServiceImpl *redis_service_impl): CommandHandler(redis_service_impl) {
        }

        brpc::RedisCommandHandlerResult Run(
            brpc::RedisConnContext *ctx,
            const std::vector<butil::StringPiece> &args,
            brpc::RedisReply *output, bool) override;
    };
}
