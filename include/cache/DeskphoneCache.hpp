// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef DESKPHONE_CACHE_HPP
#define DESKPHONE_CACHE_HPP

#include <chrono>
#include <condition_variable>
#include <dbpool/sqlite/ConnectionPool.hpp>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/// @class DeskphoneCache
///
/// @brief Object that maintains a collection of active deskphones on the Asterisk server.
///
/// This object maintains the details for the active deskphones on the Asterisk server.
class DeskphoneCache {
public:
    using clock_t = std::chrono::steady_clock;
    using time_point_t = clock_t::time_point;

protected:
    enum class SQLAction { remove, insert };

    struct HandsetData {
        SQLAction action;
        std::string aor;
        std::string endpoint;
        int64_t expiry;
    };

public:
    explicit DeskphoneCache(std::string_view filename, std::chrono::milliseconds expiry);
    ~DeskphoneCache();

    /// @brief Adds deskphone details to the cache.
    ///
    /// @param aor ID of the deskphone on the Asterisk server.
    /// @param endpoint Endpoint details of the deskphone.
    ///
    /// @return \c true if the deskphone was added to the cache.
    bool addEndpoint(std::string const &aor, std::string const &endpoint);

    /// @brief Removes the deskphone from the cache.
    ///
    /// @param aor ID of the deskphone on the asterisk server.
    /// @param ip IP address of the deskphone.
    void deleteEndpoint(std::string const &aor, std::string const &ip);

    /// @brief Invokes \c lambda on each deskphone AOR in the cache.
    ///
    /// @param lambda Lambda to invoke on each deskphone AOR.
    void forEachAOR(std::function<void(std::string const &)> const &lambda);

private:
    /// @brief Initializes the database used to contain deskphone details.
    void initializeDatabase();

    /// @brief Starts the batch writing thread.
    void startWorkThread();

    /// @brief Stops the batch writing thread.
    void stopWorkThread();

    /// @brief Performs batch writing to the database.
    void workThread();

    dbpool::sqlite::ConnectionPool connection_pool_; ///< Database connection pool for multi-threaded access.
    std::chrono::milliseconds expiry_;               ///< Liveliness for deskphone details in the database.
    std::vector<HandsetData> batch_;                 ///< Collection of record details to write to the cache database.
    std::mutex batch_mut_;                           ///< Mutex controlling access to \c batch_.
    std::thread batch_write_thread_;                 ///< Handle to the batch writing thread.
    std::atomic<bool> batch_write_run_;              ///< Flag to stop the batch writing thread.
    std::condition_variable batch_write_cv_;         ///< Control to wake the batch writing thread.
};

#endif
