//
// Created by fakzhao on 2025/8/7.
//

#pragma once
#include <cstdint>
#include <arpa/inet.h>
#include <iostream>

#include "common/socket_utils.h"

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#elif defined(__linux__)
#include <endian.h>
#else
#error "Platform not supported"
#endif

namespace blp {
    // message type
    enum MessageType : uint8_t {
        FULL_SYNC = 0x01,
        INCR_SYNC = 0x02,
        HEARTBEAT = 0x03,
        ACK = 0x04,
    };

    // message header structure
    struct MessageHeader {
        uint8_t type;
        uint64_t payload_len;
        uint64_t offset;

        void to_network_order() {
            payload_len = htobe64(payload_len);
            offset = htobe64(offset);
        }

        void from_network_order() {
            payload_len = be64toh(payload_len);
            offset = be64toh(offset);
        }
    };

    inline void start_heartbeat(int sockfd) {
        while (true) {
            MessageHeader heartbeat_header{};
            heartbeat_header.type = HEARTBEAT;
            heartbeat_header.payload_len = 0;
            heartbeat_header.offset = 0;
            heartbeat_header.to_network_order();

            if (!SocketUtils::send_all(sockfd, &heartbeat_header, sizeof(heartbeat_header))) {
                std::cerr << "[Master] Failed to send heartbeat, disconnecting...\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    inline bool send_ack(int sockfd, uint64_t offset) {
        MessageHeader ack_header{};
        ack_header.type = ACK;
        ack_header.payload_len = 0;
        ack_header.offset = offset;
        ack_header.to_network_order();
        return SocketUtils::send_all(sockfd, &ack_header, sizeof(ack_header));
    }
}
