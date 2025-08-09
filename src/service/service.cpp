//
// Created by fakzhao on 2025/8/1.
//

#include "service.h"
#include <memory>

#include "service/redis_service.h"
#include "service/cmd_get.h"
#include "service/cmd_set.h"
#include "service/cmd_auth.h"
#include "service/cmd_del.h"
#include "service/cmd_ttl.h"
#include "service/cmd_expire.h"
#include "core/db.h"

namespace blp {
    RedisServiceImpl* init_redis_service(LevelDBWrapper *db) {

        // Create RedisServiceImpl instance
        auto *redis_service_impl = new RedisServiceImpl(db);

        // auth command handler
        std::unique_ptr<CommandHandler> auth_handler = std::make_unique<AuthCommandHandler>(redis_service_impl);
        redis_service_impl->AddCommandHandler("auth", std::move(auth_handler));

        // get command handler
        std::unique_ptr<CommandHandler>  get_handler = std::make_unique<GetCommandHandler>(redis_service_impl);
        redis_service_impl->AddCommandHandler("get", std::move(get_handler));

        // set command handler
        std::unique_ptr<CommandHandler>  set_handler = std::make_unique<SetCommandHandler>(redis_service_impl);
        redis_service_impl->AddCommandHandler("set", std::move(set_handler));

        // del command handler
        std::unique_ptr<CommandHandler>  del_handler = std::make_unique<DelCommandHandler>(redis_service_impl);
        redis_service_impl->AddCommandHandler("del", std::move(del_handler));

        // ttl command handler
        std::unique_ptr<CommandHandler>  ttl_handler = std::make_unique<TtlCommandHandler>(redis_service_impl);
        redis_service_impl->AddCommandHandler("ttl", std::move(ttl_handler));

        // expire command handler
        std::unique_ptr<CommandHandler>  expire_handler = std::make_unique<ExpireCommandHandler>(redis_service_impl);
        redis_service_impl->AddCommandHandler("expire", std::move(expire_handler));

        return redis_service_impl;
    }
}
