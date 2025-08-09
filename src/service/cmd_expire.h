//
// Created by fakzhao on 2025/8/9.
//

#pragma once

#include <brpc/redis.h>

#include "cmd_base.h"
#include "redis_service.h"

namespace blp {
    class ExpireCommandHandler final : public CommandHandler {
    public:
        explicit ExpireCommandHandler(RedisServiceImpl *redis_service_impl): CommandHandler(redis_service_impl) {
        }

        brpc::RedisCommandHandlerResult Run(
            brpc::RedisConnContext *ctx,
            const std::vector<butil::StringPiece> &args,
            brpc::RedisReply *output, bool) override;
    };
}//BLPCACHE_CMD_EXPIRE_H