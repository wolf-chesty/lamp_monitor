// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/HandsetCache.hpp"

#include <ranges>

HandsetCache::HandsetCache(std::string_view filename, std::chrono::milliseconds expiry,
                           std::chrono::seconds flush_period)
    : connection_pool_(filename)
    , expiry_(expiry)
    , flush_period_(flush_period)
{
    initializeDatabase();
    startWorkThread();
}

HandsetCache::~HandsetCache()
{
    stopWorkThread();
}

void HandsetCache::initializeDatabase()
{
    connection_pool_.setPrepSql("PRAGMA journal_mode=WAL; PRAGMA busy_timeout=3000");

    auto connection = connection_pool_.getConnection();
    connection.exec("CREATE TABLE IF NOT EXISTS endpoints ("
                    "aor TEXT NOT NULL,"
                    " ip TEXT NOT NULL,"
                    " expiry INT64,"
                    " PRIMARY KEY(aor, ip));");

    connection.exec("UPDATE endpoints SET expiry = 0;");
}

bool HandsetCache::addEndpoint(std::string const &aor, std::string const &ip)
{
    // Query table for unexpired handset
    auto connection = connection_pool_.getConnection();
    auto check = connection.getStmt("SELECT EXISTS(SELECT 1 FROM endpoints WHERE aor = ? AND ip = ? AND expiry > ?);");
    check.bindText(1, aor);

    auto const fields = ip | std::views::split('/') | std::views::transform([](auto &&r) {
                            return std::string_view(&*r.begin(), std::ranges::distance(r));
                        });
    // Pull IP out of the field
    int i = 0;
    for (auto const field : fields) {
        if (i++ == 2) {
            check.bindText(2, field);
            break;
        }
    }

    auto const now = clock_t::now();
    check.bindInt64(3, now.time_since_epoch().count());
    auto ret = check.execute();
    assert(ret == dbpool::PreparedStmt::ReturnCode::Row);
    auto const found_rec{check.getBool(0)};

    // Enqueue the endpoint for add
    std::lock_guard const lock(batch_mut_);
    batch_.emplace_back(SQLAction::insert, aor, ip, (now + expiry_).time_since_epoch().count());
    batch_write_cv_.notify_one();

    return !found_rec;
}

void HandsetCache::deleteEndpoint(std::string const &aor, std::string const &ip)
{
    std::lock_guard const lock(batch_mut_);
    batch_.emplace_back(SQLAction::remove, aor, ip, 0);
    batch_write_cv_.notify_one();
}

void HandsetCache::forEachAOR(std::function<void(std::string_view)> const &lambda)
{
    auto connection = connection_pool_.getConnection();
    auto select_stmt = connection.getStmt("SELECT DISTINCT aor FROM endpoints WHERE expiry > ?;");
    auto const now = clock_t::now();
    select_stmt.bindInt64(1, now.time_since_epoch().count());
    while (select_stmt.execute() == dbpool::PreparedStmt::ReturnCode::Row) {
        lambda(select_stmt.getText(0));
    }
}

void HandsetCache::startWorkThread()
{
    batch_write_run_ = true;
    batch_write_thread_ = std::thread(&HandsetCache::workThread, this);

    pthread_setname_np(batch_write_thread_.native_handle(), "db_writer");
}

void HandsetCache::stopWorkThread()
{
    batch_write_run_ = false;
    batch_write_cv_.notify_one();

    assert(batch_write_thread_.joinable());
    batch_write_thread_.join();
}

void HandsetCache::workThread()
{
    // Rather than recompiling prepared statements for every iteration of this service thread this function will
    // checkout a database connection keep it with this thread.
    auto connection = connection_pool_.getConnection();
    auto begin = connection.getStmt("BEGIN;");
    auto del_aor = connection.getStmt("UPDATE endpoints SET expiry = 0 WHERE aor = ? AND ip = ?;");
    auto add_aor = connection.getStmt("INSERT INTO endpoints (aor, ip, expiry) VALUES (?, ?, ?)"
                                      " ON CONFLICT(aor, ip) DO UPDATE SET expiry = excluded.expiry;");
    auto expunge = connection.getStmt("DELETE FROM endpoints WHERE expiry < ?;");
    auto commit = connection.getStmt("COMMIT;");
    auto checkpoint = connection.getStmt("PRAGMA wall_checkpoint(TRUNCATE);");

    flush_time_ = clock_t::now() + flush_period_;

    std::vector<HandsetData> batch;

    while (batch_write_run_) {
        std::unique_lock lock(batch_mut_);
        batch_write_cv_.wait(lock, [this]() -> bool { return !batch_write_run_ || !batch_.empty(); });
        std::swap(batch_, batch);
        lock.unlock();

        // Begin transaction
        auto ret = begin.execute();
        assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
        begin.reset();

        for (auto const &data : batch) {
            if (data.action == SQLAction::remove) {
                del_aor.bindText(1, data.aor);
                del_aor.bindText(2, data.ip);
                ret = del_aor.execute();
                assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
                del_aor.reset();
            }
            else {
                add_aor.bindText(1, data.aor);

                // Split the IP into its separate fields
                auto const fields = data.ip | std::views::split('/') | std::views::transform([](auto &&r) {
                                        return std::string_view(&*r.begin(), std::ranges::distance(r));
                                    });
                // Pull IP out of the field
                int i = 0;
                for (auto const field : fields) {
                    if (i++ == 2) {
                        add_aor.bindText(2, field);
                        break;
                    }
                }

                add_aor.bindInt64(3, data.expiry);

                ret = add_aor.execute();
                assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
                add_aor.reset();
            }
        }
        batch.clear();

        // Expunge expired endpoints
        auto const now = clock_t::now();
        expunge.bindInt64(1, now.time_since_epoch().count());
        ret = expunge.execute();
        assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
        expunge.reset();

        // Commit the transactions
        ret = commit.execute();
        assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
        commit.reset();

        if (now > flush_time_) {
            ret = checkpoint.execute();
            assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
            checkpoint.reset();

            flush_time_ = now + flush_period_;
        }
    }
}
