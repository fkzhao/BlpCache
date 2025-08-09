//
// Created by fakzhao on 2025/8/7.
//

#pragma once
#include <cstdint>
#include <arpa/inet.h>
#include <iostream>
#include <thread>

#include "common/socket_utils.h"
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

    class DataEntity {
    public:
        DataEntity() = default;

        DataEntity(uint64_t seq, const std::string &k, const std::string &v, const std::string &c)
            : sequence_(seq), key_(k), value_(v), cmd_(c) {
        }

        // getters/setters
        uint64_t sequence() const { return sequence_; }
        void setSequence(uint64_t s) { sequence_ = s; }
        const std::string &cmd() const { return cmd_; }
        void setCmd(const std::string &c) { cmd_ = c; }
        const std::string &key() const { return key_; }
        void setKey(const std::string &k) { key_ = k; }
        const std::string &value() const { return value_; }
        void setValue(const std::string &v) { value_ = v; }

        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> buf;
            uint64_t seq_net = htobe64(sequence_);
            appendBytes(buf, &seq_net, sizeof(seq_net));

            uint32_t cmd_len = htonl(static_cast<uint32_t>(cmd_.size()));
            appendBytes(buf, &cmd_len, sizeof(cmd_len));
            appendBytes(buf, cmd_.data(), cmd_.size());

            uint32_t key_len = htonl(static_cast<uint32_t>(key_.size()));
            appendBytes(buf, &key_len, sizeof(key_len));
            appendBytes(buf, key_.data(), key_.size());

            uint32_t val_len = htonl(static_cast<uint32_t>(value_.size()));
            appendBytes(buf, &val_len, sizeof(val_len));
            appendBytes(buf, value_.data(), value_.size());
            return buf;
        }

        bool deserialize(const uint8_t *data, size_t len) {
            size_t off = 0;
            if (len < sizeof(uint64_t)) return false;
            sequence_ = be64toh(*reinterpret_cast<const uint64_t*>(data + off));
            off += sizeof(uint64_t);

            if (off + sizeof(uint32_t) > len) return false;
            uint32_t cmd_len = ntohl(*reinterpret_cast<const uint32_t*>(data + off));
            off += sizeof(uint32_t);
            if (off + cmd_len > len) return false;
            cmd_.assign(reinterpret_cast<const char *>(data + off), cmd_len);
            off += cmd_len;

            if (off + sizeof(uint32_t) > len) return false;
            uint32_t key_len = ntohl(*reinterpret_cast<const uint32_t*>(data + off));
            off += sizeof(uint32_t);
            if (off + key_len > len) return false;
            key_.assign(reinterpret_cast<const char *>(data + off), key_len);
            off += key_len;

            if (off + sizeof(uint32_t) > len) return false;
            uint32_t val_len = ntohl(*reinterpret_cast<const uint32_t*>(data + off));
            off += sizeof(uint32_t);
            if (off + val_len > len) return false;
            value_.assign(reinterpret_cast<const char *>(data + off), val_len);
            off += val_len;
            return true;
        }
    private:
        uint64_t sequence_{0};
        std::string key_;
        std::string value_;
        std::string cmd_;

        static void appendBytes(std::vector<uint8_t> &buf, const void *p, size_t n) {
            const uint8_t *b = reinterpret_cast<const uint8_t *>(p);
            buf.insert(buf.end(), b, b + n);
        }
    };

    // message header
    class Message {
    public:
        MessageType type = ACK;
        uint16_t magic = 0x9798; // custom magic number
        uint64_t len = 0; // length of the payload
        uint64_t offset = 0; // offset for incremental sync
        std::vector<uint8_t> payload;

        static Message fromDataEntity(MessageType t, const DataEntity &e) {
            Message m;
            m.type = t;
            m.offset = e.sequence();
            m.magic = 0x9798;
            m.payload = e.serialize();
            m.len = m.payload.size();
            return m;
        }

        bool toDataEntity(DataEntity& e) const {
            return e.deserialize(payload.data(), payload.size());
        }

        bool sendToSocket(int sockfd) const {
            const auto type_byte = static_cast<uint8_t>(type);
            if (!SocketUtils::send_all(sockfd, &type_byte, sizeof(type_byte))) return false;

            const auto offset_byte = static_cast<uint8_t>(offset);
            if (!SocketUtils::send_all(sockfd, &offset_byte, sizeof(offset_byte))) return false;

            const uint16_t magic_byte = htons(magic);
            if (!SocketUtils::send_all(sockfd, &magic_byte, sizeof(magic_byte))) return false;

            const uint64_t payload_len_net = htobe64(static_cast<uint64_t>(len));
            if (!SocketUtils::send_all(sockfd, &payload_len_net, sizeof(payload_len_net))) return false;

            //
            if (!payload.empty()) {
                if (!SocketUtils::send_all(sockfd, payload.data(), payload.size())) return false;
            }
            return true;
        }

        static bool recvFromSocket(int sockfd, Message& msg) {
            uint8_t type_byte;
            if (!SocketUtils::recv_all(sockfd, &type_byte, sizeof(type_byte))) return false;
            msg.type = static_cast<MessageType>(type_byte);

            uint8_t offset_byte;
            if (!SocketUtils::recv_all(sockfd, &offset_byte, sizeof(offset_byte))) return false;
            msg.offset = be64toh(offset_byte);


            uint16_t magic_byte;;
            if (!SocketUtils::recv_all(sockfd, &magic_byte, sizeof(magic_byte))) return false;
            msg.magic = be64toh(magic_byte);

            uint64_t payload_len_net;
            if (!SocketUtils::recv_all(sockfd, &payload_len_net, sizeof(payload_len_net))) return false;
            const uint64_t payload_len = be64toh(payload_len_net);

            msg.payload.resize(payload_len);
            if (payload_len > 0) {
                if (!SocketUtils::recv_all(sockfd, msg.payload.data(), payload_len)) return false;
            }
            return true;
        }
    };

    inline void start_heartbeat(int sockfd) {
        while (true) {
            Message message;
            message.type = HEARTBEAT;
            if (message.sendToSocket(sockfd)) {
                std::cerr << "[Master] Failed to send heartbeat, disconnecting...\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    inline bool send_ack(int sockfd, uint64_t offset) {
        Message ack_header{};
        ack_header.type = ACK;
        return ack_header.sendToSocket(sockfd);
    }
}
