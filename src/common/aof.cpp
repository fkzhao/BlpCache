//
// Created by fakzhao on 2025/8/9.
//

#include "aof.h"

#include <utility>

namespace blp::aof {
    Aof::Aof(std::string path, const Options &opts)
            : path_(std::move(path)), opts_(opts), stop_flag_(false), fd_(-1), writer_started_(false)
    {
        openFile();
        startWriter();
    }

    bool Aof::appendCommandAsync(const std::vector<std::string>& parts) {
        std::string resp = serializeToRESP(parts);
        return appendRawAsync(std::move(resp));
    }

    // 将已经是 RESP 格式字符串直接入队（非阻塞）
    bool Aof::appendRawAsync(std::string resp_payload) {
        std::unique_lock<std::mutex> lk(mu_);
        if (stop_flag_) return false;
        if (queue_.size() >= opts_.max_queue_entries) return false;
        queue_.push_back(std::move(resp_payload));
        lk.unlock();
        cv_.notify_one();
        notifyListeners();
        return true;
    }

    // 阻塞入队版本（直到有空间或 stop）
    bool Aof::appendCommandBlocking(const std::vector<std::string>& parts) {
        std::string resp = serializeToRESP(parts);
        return appendRawBlocking(std::move(resp));
    }

    bool Aof::appendRawBlocking(std::string resp_payload) {
        std::unique_lock<std::mutex> lk(mu_);
        cv_space_.wait(lk, [&]{ return stop_flag_ || queue_.size() < opts_.max_queue_entries; });
        if (stop_flag_) return false;
        queue_.push_back(std::move(resp_payload));
        lk.unlock();
        cv_.notify_one();
        notifyListeners();
        return true;
    }

    // 注册监听器（当新数据入队时会触发）
    void Aof::registerListener(Listener cb) {
        std::lock_guard<std::mutex> lk(listener_mu_);
        listeners_.push_back(std::move(cb));
    }

    // 获取当前已安全写入磁盘的偏移（approximate）
    uint64_t Aof::getCurrentOffset() const {
        return written_offset_.load();
    }

    // 启动写线程（safe to call multiple times）
    void Aof::startWriter() {
        std::lock_guard<std::mutex> lk(start_mu_);
        if (writer_started_) return;
        stop_flag_ = false;
        writer_thread_ = std::thread(&Aof::writerLoop, this);
        writer_started_ = true;
    }

    // 优雅停止并 flush + fsync（如果配置要求）
    void Aof::stop() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (stop_flag_) return;
            stop_flag_ = true;
        }
        cv_.notify_one();
        cv_space_.notify_all();
        if (writer_thread_.joinable()) writer_thread_.join();
        writer_started_ = false;
        if (fd_ >= 0) {
            if (opts_.durability_mode > 0) ::fsync(fd_);
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool Aof::recoverFromAOF(const CmdExecutor& exec_fn, bool truncate_incomplete_tail) {
        int rfd = ::open(path_.c_str(), O_RDONLY | O_CLOEXEC);
        if (rfd < 0) {
            if (errno == ENOENT) return true;
            std::cerr << "recoverFromAOF: open failed: " << strerror(errno) << "\n";
            return false;
        }

        struct stat st{};
        if (fstat(rfd, &st) != 0) {
            ::close(rfd);
            std::cerr << "recoverFromAOF: fstat failed\n";
            return false;
        }
        off_t filesize = st.st_size;
        if (filesize == 0) {
            ::close(rfd);
            return true;
        }

        const size_t BUF = 64 * 1024;
        std::vector<char> buf(BUF);
        std::string stream_buf;
        ssize_t n;
        off_t total_read = 0;
        uint64_t last_good_offset = 0; // 最后完整命令的文件结束偏移

        while ((n = ::read(rfd, buf.data(), buf.size())) > 0) {
            total_read += n;
            stream_buf.append(buf.data(), static_cast<size_t>(n));
            size_t parsed = 0;
            while (true) {
                std::vector<std::string> parts;
                size_t consumed = 0;
                bool ok = tryParseRESPArray(stream_buf.data() + parsed, stream_buf.size() - parsed, parts, consumed);
                if (!ok) break; // not enough data for next command
                exec_fn(parts);
                parsed += consumed;
                last_good_offset += consumed;
            }
            if (parsed > 0) {
                stream_buf.erase(0, parsed);
            }
        }

        if (n < 0) {
            std::cerr << "recoverFromAOF: read failed: " << strerror(errno) << "\n";
            ::close(rfd);
            return false;
        }

        if (!stream_buf.empty()) {
            if (truncate_incomplete_tail) {
                if (ftruncate(rfd, static_cast<off_t>(last_good_offset)) != 0) {
                    std::cerr << "recoverFromAOF: ftruncate failed: " << strerror(errno) << "\n";
                    ::close(rfd);
                    return false;
                }
                std::cerr << "recoverFromAOF: truncated incomplete tail of size " << stream_buf.size() << "\n";
            } else {
                std::cerr << "recoverFromAOF: ignoring incomplete tail of size " << stream_buf.size() << "\n";
            }
        }

        off_t cur = lseek(rfd, 0, SEEK_END);
        if (cur >= 0) written_offset_.store(static_cast<uint64_t>(cur));
        ::close(rfd);
        return true;
    }

    bool Aof::readCommandsFromOffset(uint64_t offset, const CmdExecutor& exec_fn) const {
        int rfd = ::open(path_.c_str(), O_RDONLY | O_CLOEXEC);
        if (rfd < 0) {
            std::cerr << "readCommandsFromOffset: open failed: " << strerror(errno) << "\n";
            return false;
        }
        off_t res = lseek(rfd, static_cast<off_t>(offset), SEEK_SET);
        if (res == (off_t)-1) {
            std::cerr << "readCommandsFromOffset: lseek failed: " << strerror(errno) << "\n";
            ::close(rfd);
            return false;
        }

        const size_t BUF = 64 * 1024;
        std::vector<char> buf(BUF);
        std::string stream_buf;
        ssize_t n;
        bool any_parsed = false;

        while ((n = ::read(rfd, buf.data(), buf.size())) > 0) {
            stream_buf.append(buf.data(), static_cast<size_t>(n));
            size_t parsed = 0;
            while (true) {
                std::vector<std::string> parts;
                size_t consumed = 0;
                bool ok = tryParseRESPArray(stream_buf.data() + parsed, stream_buf.size() - parsed, parts, consumed);
                if (!ok) break;
                exec_fn(parts);
                parsed += consumed;
                any_parsed = true;
            }
            if (parsed > 0) stream_buf.erase(0, parsed);
        }

        if (n < 0) {
            std::cerr << "readCommandsFromOffset: read failed: " << strerror(errno) << "\n";
            ::close(rfd);
            return false;
        }

        ::close(rfd);
        return true;
    }

    void Aof::writerLoop() {
        using clock = std::chrono::steady_clock;
        auto last_fsync = clock::now();

        std::vector<std::string> batch;
        batch.reserve(1024);

        while (true) {
            {
                std::unique_lock<std::mutex> lk(mu_);
                if (!stop_flag_ && queue_.empty()) {
                    cv_.wait_for(lk, opts_.flush_interval);
                }
                // move up to batch limits
                size_t batch_bytes = 0;
                while (!queue_.empty() && batch.size() < opts_.max_batch_entries && batch_bytes < opts_.max_batch_bytes) {
                    batch.push_back(std::move(queue_.front()));
                    batch_bytes += batch.back().size();
                    queue_.pop_front();
                }
                if (queue_.size() < opts_.max_queue_entries / 2) {
                    cv_space_.notify_all();
                }
            }

            if (batch.empty()) {
                if (stop_flag_) break;
                if (opts_.durability_mode == 2) {
                    auto now = clock::now();
                    if (now - last_fsync >= opts_.periodic_fsync_interval && fd_ >= 0) {
                        ::fsync(fd_);
                        last_fsync = now;
                    }
                }
                continue;
            }

            // prepare iovec for writev
            std::vector<iovec> iovs;
            iovs.reserve(batch.size());
            // keep strings alive in prepared vector
            std::vector<std::string> prepared;
            prepared.reserve(batch.size());
            size_t total_bytes = 0;
            for (auto &s : batch) {
                prepared.push_back(std::move(s));
            }
            for (auto &s : prepared) {
                iovec iv;
                iv.iov_base = const_cast<char*>(s.data());
                iv.iov_len = s.size();
                iovs.push_back(iv);
                total_bytes += s.size();
            }

            // writev in slices (avoid exceeding system IOV_MAX)
            size_t idx = 0;
            const int IOV_MAX_SLICE = 64; // conservative
            while (idx < iovs.size()) {
                int slice_len = static_cast<int>(std::min<size_t>(IOV_MAX_SLICE, iovs.size() - idx));
                ssize_t n = ::writev(fd_, iovs.data() + idx, slice_len);
                if (n < 0) {
                    if (errno == EINTR) continue;
                    std::cerr << "writerLoop: writev failed: " << strerror(errno) << "\n";
                    stop_flag_ = true;
                    break;
                }
                // advance iovs by n bytes
                ssize_t remaining = n;
                while (remaining > 0 && idx < iovs.size()) {
                    if (remaining >= static_cast<ssize_t>(iovs[idx].iov_len)) {
                        remaining -= static_cast<ssize_t>(iovs[idx].iov_len);
                        ++idx;
                    } else {
                        // adjust current iov base/len (the next writev slice will consider this)
                        iovs[idx].iov_base = static_cast<char*>(iovs[idx].iov_base) + remaining;
                        iovs[idx].iov_len -= remaining;
                        remaining = 0;
                    }
                }
            }

            // update offset
            off_t cur = lseek(fd_, 0, SEEK_CUR);
            if (cur != (off_t)-1) written_offset_.store(static_cast<uint64_t>(cur));

            // fsync policy
            if (opts_.durability_mode == 1) {
                ::fsync(fd_);
                last_fsync = clock::now();
            } else if (opts_.durability_mode == 2) {
                auto now = clock::now();
                if (now - last_fsync >= opts_.periodic_fsync_interval) {
                    ::fsync(fd_);
                    last_fsync = now;
                }
            }

            batch.clear();

            if (stop_flag_) {
                // empty remaining queue before exit
                std::unique_lock<std::mutex> lk(mu_);
                if (queue_.empty()) break;
            }
        }

        if (opts_.durability_mode > 0) ::fsync(fd_);
    }

    void Aof::openFile() {
        fd_ = ::open(path_.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
        if (fd_ < 0) {
            std::cerr << "openFile failed: " << strerror(errno) << "\n";
            throw std::runtime_error("open aof failed");
        }
        off_t cur = lseek(fd_, 0, SEEK_END);
        if (cur >= 0) written_offset_.store(static_cast<uint64_t>(cur));
    }

    void Aof::notifyListeners() {
        std::vector<Listener> copy;
        {
            std::lock_guard<std::mutex> lk(listener_mu_);
            copy = listeners_;
        }
        for (auto &cb : copy) {
            try {
                cb();
            } catch (...) { /* ignore */ }
        }
    }

    // RESP 序列化（数组 of bulk strings）
    std::string Aof::serializeToRESP(const std::vector<std::string>& parts) {
        std::string out;
        out.reserve(64 + parts.size() * 16);
        out.push_back('*');
        out.append(std::to_string(parts.size()));
        out.append("\r\n");
        for (const auto &p : parts) {
            out.push_back('$');
            out.append(std::to_string(p.size()));
            out.append("\r\n");
            out.append(p);
            out.append("\r\n");
        }
        return out;
    }

    // 尝试从 buffer 中解析一个 RESP 数组（完整），
    // 如果成功，parts 被填充，并返回 true，consumed 为该帧字节数；
    // 如果数据不足以组成完整帧，返回 false（不修改 parts / consumed）。
    // 注意：该函数不会尝试处理 RESP 错误类型或简单字符串——AOF 中存储的是 Array of Bulk Strings。
    bool Aof::tryParseRESPArray(const char* buf, size_t len, std::vector<std::string>& parts, size_t& consumed) {
        consumed = 0;
        parts.clear();
        size_t pos = 0;

        auto need = [&](size_t n)->bool { return pos + n <= len; };

        // parse '*<num>\r\n'
        if (!need(1)) return false;
        if (buf[pos] != '*') return false; // invalid start for our expected frame
        ++pos;
        // read until \r\n
        size_t rn = findCRLF(buf, len, pos);
        if (rn == std::string::npos) return false;
        std::string numstr(buf + pos, rn - pos);
        int num;
        try {
            num = std::stoi(numstr);
        } catch (...) { return false; }
        pos = rn + 2; // past \r\n

        // parse num bulk strings
        parts.reserve(num);
        for (int i = 0; i < num; ++i) {
            // expect '$'
            if (!need(1)) return false;
            if (buf[pos] != '$') return false;
            ++pos;
            size_t rn2 = findCRLF(buf, len, pos);
            if (rn2 == std::string::npos) return false;
            std::string lenstr(buf + pos, rn2 - pos);
            long slen;
            try {
                slen = std::stol(lenstr);
            } catch (...) { return false; }
            pos = rn2 + 2;
            if (slen < 0) {
                // null bulk string - treat as empty string here
                parts.emplace_back();
                continue;
            }
            if (!need(static_cast<size_t>(slen) + 2)) return false; // not enough bytes for data + \r\n
            parts.emplace_back(std::string(buf + pos, static_cast<size_t>(slen)));
            pos += static_cast<size_t>(slen);
            // expect CRLF
            if (!need(2)) return false;
            if (buf[pos] != '\r' || buf[pos+1] != '\n') return false;
            pos += 2;
        }

        consumed = pos;
        return true;
    }

    size_t Aof::findCRLF(const char* buf, size_t len, size_t start) {
        for (size_t i = start; i + 1 < len; ++i) {
            if (buf[i] == '\r' && buf[i+1] == '\n') return i;
        }
        return std::string::npos;
    }
}