#include <brpc/server.h>
#include <brpc/redis.h>
#include <butil/crc32c.h>
#include <butil/strings/string_split.h>
#include <gflags/gflags.h>
#include <thread>
#include <string>
#include <leveldb/db.h>

#include <butil/time.h>

#include "common/config.h"
#include "service/service.h"
#include "common/sequence_number.h"
#include "replica/server.h"
#include "replica/replicate.h"
#include "common/ring_buffer.h"
#include "common/aof.h"

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    // load configuration
    const std::string conf_file = std::string(getenv("BLP_HOME")) + "/conf/blp.conf";
    if (!blp::config::init(conf_file)) {
        fprintf(stderr, "error read config file. \n");
        return -1;
    }

    // init aof
    const std::string aof_file = std::string(getenv("BLP_HOME")) + "/data/appendonly.aof";
    blp::aof::Options opts;
    opts.max_queue_entries = 1 << 14;
    opts.max_batch_entries = 256;
    opts.max_batch_bytes = 128 * 1024;
    opts.flush_interval = std::chrono::milliseconds(100);
    opts.durability_mode = 1;
    blp::aof::Aof aof(aof_file, opts);

    std::cout <<"BLP Cache Start On: " << blp::config::port << std::endl;
    // init db
    const std::string db_path = std::string(getenv("BLP_HOME")) + "/data/db";
    blp::LevelDBWrapper* db = blp::init_db(db_path, aof);
    if (db == nullptr) {
        fprintf(stderr, "error init db. \n");
        return -1;
    }

    // config brpc server
    LOG(INFO) << "Starting Redis Service...";
    brpc::Server redis_server;
    brpc::ServerOptions redis_server_options;

    // init redis service
    const auto redis_service_impl = blp::init_redis_service(db);
    redis_server_options.redis_service = redis_service_impl;

    // start server
    if (redis_server.Start(blp::config::port, &redis_server_options) != 0) {
        LOG(ERROR) << "Fail to start server";
        return -1;
    }
    LOG(INFO) << "Redis Service started on port " << blp::config::port;

    //
    if (blp::config::model == "master") {
        LOG(INFO) << "Starting Replication Service...";
        std::string host = "127.0.0.1";
        auto replicationServer = blp::ReplicationServer(aof);
        std::thread master_server([&]{ replicationServer.startServer(blp::config::replica_port); });
        master_server.detach();
    } else if (blp::config::model == "replicate") {
        LOG(INFO) << "Starting Replication Client...";
        std::string host = "127.0.0.1";
        auto replicationClient = blp::ReplicationClient();
        std::thread replica_client([&]{ replicationClient.startReplica(host.data(), blp::config::replica_port, db); });
        replica_client.detach();
    }  else {
        LOG(ERROR) << "Unknown model: " << blp::config::model;
        return -1;
    }
    redis_server.RunUntilAskedToQuit();
    aof.stop();
    return 0;
}