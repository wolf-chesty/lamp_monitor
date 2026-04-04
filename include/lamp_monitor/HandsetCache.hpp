// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef HANDSET_CACHE_HPP
#define HANDSET_CACHE_HPP

#include <chrono>
#include <condition_variable>
#include <dbpool/sqlite/ConnectionPool.hpp>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class HandsetCache {
public:
    using clock_t = std::chrono::steady_clock;
    using time_point_t = clock_t::time_point;

protected:
    enum class SQLAction { remove, insert };

    struct HandsetData {
        SQLAction action;
        std::string aor;
        std::string ip;
        int64_t expiry;
    };

public:
    explicit HandsetCache(std::string_view filename, std::chrono::milliseconds expiry, std::chrono::seconds flush_period);
    ~HandsetCache();

    bool addEndpoint(std::string const &aor, std::string const &ip);
    void deleteEndpoint(std::string const &aor, std::string const &ip);

    void forEachAOR(std::function<void(std::string_view)> const &lambda);

private:
    void initializeDatabase();

    void startWorkThread();
    void stopWorkThread();
    void workThread();

    dbpool::sqlite::ConnectionPool connection_pool_;
    std::chrono::milliseconds expiry_;
    time_point_t flush_time_;
    std::chrono::seconds flush_period_;
    std::vector<HandsetData> batch_;
    std::mutex batch_mut_;
    std::thread batch_write_thread_;
    std::atomic<bool> batch_write_run_;
    std::condition_variable batch_write_cv_;
};

#endif
