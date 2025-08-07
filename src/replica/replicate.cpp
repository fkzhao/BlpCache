//
// Created by fakzhao on 2025/8/6.
//

#include "replicate.h"
#include <thread>
#include "common/socket_utils.h"
#include "protocol.h"

namespace blp {
    std::atomic<bool> r_running(true);

    void start_replica(char *host, uint16_t port) {
        while (r_running) {
            try {
                int sockfd = SocketUtils::create_client_socket(host, port);
                std::cout << "[Replica] Connected to Master at " << host << ":" << port << std::endl;
                // send heartbeat
                std::thread heartbeat(start_heartbeat, sockfd);
                heartbeat.detach();

                //
                while (r_running) {
                    MessageHeader header{};
                    if (!SocketUtils::recv_all(sockfd, &header, sizeof(header))) {
                        std::cerr << "[Replica] Failed to receive header, disconnecting...\n";
                        break;
                    }
                    header.from_network_order();

                    if (header.type == ACK) {
                        std::cout << "[Replica] Received ACK from Master, starting sync...\n";
                        continue;
                    }

                    std::string payload(header.payload_len, 0);
                    if (!SocketUtils::recv_all(sockfd, payload.data(), header.payload_len)) {
                        std::cerr << "[Replica] Failed to receive payload, disconnecting...\n";
                        break;
                    }

                    if (header.type == FULL_SYNC) {
                        std::cout << "[Replica] Receiving FULL_SYNC chunk offset " << header.offset << " size " << header.
                                payload_len << std::endl;
                    } else if (header.type == INCR_SYNC) {
                        std::cout << "[Replica] Receiving INCR_SYNC offset " << header.offset << " size " << header.
                                payload_len << std::endl;;
                    } else if (header.type == HEARTBEAT) {
                        std::cout << "[Replica] Receiving HEARTBEAT offset " << header.offset << " size " << header.
                               payload_len << std::endl;
                    } else if (header.type == ACK) {
                        std::cout << "[Replica] Receiving ACK offset " << header.offset << " size " << header.
                               payload_len << std::endl;;
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
