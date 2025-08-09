//
// Created by fakzhao on 2025/8/6.
//

#include "replicate.h"
#include <thread>
#include "common/socket_utils.h"
#include "protocol.h"
#include "core/db.h"

namespace blp {

    void ReplicationClient::startReplica(char *host, uint16_t port, LevelDBWrapper *db) {
        while (true) {
            try {
                int sockfd = SocketUtils::create_client_socket(host, port);
                std::cout << "[Replica] Connected to Master at " << host << ":" << port << std::endl;
                // send heartbeat
                std::thread heartbeat(start_heartbeat, sockfd);
                heartbeat.detach();

                //
                while (true) {
                    Message header;
                    Message::recvFromSocket(sockfd,header);

                    if (header.type == FULL_SYNC) {
                        std::cout << "[Replica] Receiving FULL_SYNC chunk offset " << header.offset << " size " << header.
                                len << std::endl;
                    } else if (header.type == INCR_SYNC) {
                        std::cout << "[Replica] Receiving INCR_SYNC offset " << header.offset << " size " << header.
                                len << std::endl;;
                        DataEntity entry;
                        header.toDataEntity(entry);
                        std::cout << "[Replica] Processing data entry: sequence " << entry.sequence()
                                  << ", key " << entry.key() << ", value " << entry.value() << ", cmd " << entry.cmd() << std::endl;
                        std::string cmd = entry.cmd();
                        if (cmd.empty()) {
                            continue;
                        }
                        if (cmd == "DEL") {
                            std::cout << "[Replica] Deleting key " << entry.key() << std::endl;
                            bool res = db->remove(entry.key());
                            std::cout << "[Replica] Deleting data entry from DB: sequence " << entry.sequence()
                                      << ", key " << entry.key() << ", res " << res << std::endl;
                        } else if (cmd == "SET") {
                            bool res = db->put(entry.key(), entry.value());
                            std::cout << "[Replica] Setting key " << entry.key() << " to value " << entry.value() << " res " << res << std::endl;
                        }
                    } else if (header.type == HEARTBEAT) {
                        std::cout << "[Replica] Receiving HEARTBEAT offset " << header.offset << " size " << header.
                               len << std::endl;
                    } else if (header.type == ACK) {
                        std::cout << "[Replica] Receiving ACK offset " << header.offset << " size " << header.
                               len << std::endl;;
                    } else {
                        std::cerr << "[Replica] Unknown message type: " << int(header.type) << std::endl;
                    }
                }

                close(sockfd);
                std::cout << "[Replica] Connection closed. Reconnecting in 3 seconds...\n";
                std::this_thread::sleep_for(std::chrono::seconds(3));
            } catch (const std::exception &e) {
                std::cerr << "[Replica] Exception: " << e.what() << ", retrying in 3 seconds...\n";
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
    }
}
