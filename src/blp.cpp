#include <brpc/server.h>
#include <brpc/redis.h>
#include <butil/crc32c.h>
#include <butil/strings/string_split.h>
#include <gflags/gflags.h>
#include <memory>
#include <string>
#include <leveldb/db.h>

#include <butil/time.h>

#include "common/config.h"
#include "service/service.h"
#include "service/cmd_set.h"
#include "service/cmd_get.h"
#include "service/cmd_del.h"
#include "service/cmd_auth.h"

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    // load configuration
    const std::string conf_file = std::string(getenv("BLP_HOME")) + "/conf/blp.conf";
    if (!blp::config::init(conf_file.c_str(), true, true, true)) {
        fprintf(stderr, "error read config file. \n");
        return -1;
    }

    // config brpc server
    LOG(INFO) << "Starting service...";
    brpc::Server server;
    brpc::ServerOptions server_options;

    // init redis service
    // blp::init_service(&server_options);
    const auto rsimpl = blp::init_redis_service();
    server_options.redis_service = rsimpl;
    server_options.idle_timeout_sec = 60; // Set idle timeout to 60 seconds

    // start server
    if (server.Start(6479, &server_options) != 0) {
        LOG(ERROR) << "Fail to start server";
        return -1;
    }
    server.RunUntilAskedToQuit();
    return 0;
}