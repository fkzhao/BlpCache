//
// Created by fakzhao on 2025/8/9.
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sstream>


namespace blp::aof {
    // durability_mode:
    // 0 = never fsync
    // 1 = fsync every batch write
    // 2 = periodic fsync every periodic_fsync_interval
    struct Options {
        size_t max_queue_entries = 1 << 16;
        size_t max_batch_entries = 512;
        size_t max_batch_bytes = 64 * 1024;
        std::chrono::milliseconds flush_interval{200}; // wake writer periodically
        int durability_mode = 1;
        std::chrono::milliseconds periodic_fsync_interval{1000};
    };

    class Aof {
    public:
        using CmdExecutor = std::function<void(const std::vector<std::string>&)>;
        using Listener = std::function<void()>;


        explicit Aof(std::string path, const Options &opts = Options());

        bool appendCommandAsync(const std::vector<std::string>& parts);

        bool appendRawAsync(std::string resp_payload);

        bool appendCommandBlocking(const std::vector<std::string>& parts);

        bool appendRawBlocking(std::string resp_payload);

        void registerListener(Listener cb);

        uint64_t getCurrentOffset() const;

        void startWriter();

        void stop();

        bool recoverFromAOF(const CmdExecutor& exec_fn, bool truncate_incomplete_tail = true);

        bool readCommandsFromOffset(uint64_t offset, const CmdExecutor& exec_fn) const;

        ~Aof() {
            stop();
        }
    private:
        std::string path_;
        Options opts_;

        mutable std::mutex mu_;
        std::condition_variable cv_;
        std::condition_variable cv_space_;
        std::deque<std::string> queue_;
        std::atomic<bool> stop_flag_;

        std::mutex listener_mu_;
        std::vector<Listener> listeners_;

        int fd_;
        std::thread writer_thread_;
        bool writer_started_;
        std::mutex start_mu_;
        std::mutex listener_call_mu_;

        std::atomic<uint64_t> written_offset_{0};

        void writerLoop();

        void openFile();

        void notifyListeners();


        static std::string serializeToRESP(const std::vector<std::string>& parts);

        static bool tryParseRESPArray(const char* buf, size_t len, std::vector<std::string>& parts, size_t& consumed);

        static size_t findCRLF(const char* buf, size_t len, size_t start);

    };

}