// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "cache/DeskphoneCache.hpp"

#include <charconv>
#include <ranges>
#include <syslog.h>

DeskphoneCache::DeskphoneCache(std::string_view filename, std::chrono::milliseconds expiry)
    : connection_pool_(filename)
    , expiry_(expiry)
{
    syslog(LOG_DEBUG, "DeskphoneCache::DeskphoneCache(%s, %ld)", filename.data(), expiry_.count());

    initializeDatabase();
    startWriteThread();
}

DeskphoneCache::~DeskphoneCache()
{
    syslog(LOG_DEBUG, "DeskphoneCache::~DeskphoneCache()");

    stopWriteThread();

    auto connection = connection_pool_.getConnection();

    // Expunge expired endpoints
    auto expunge = connection.getStmt("DELETE FROM endpoints WHERE expiry < ?;");
    auto const now = clock_t::now();
    expunge.bindInt64(1, now.time_since_epoch().count());
    auto ret = expunge.execute();
    assert(ret == dbpool::PreparedStmt::ReturnCode::Done);

    // Truncate the database file
    ret = connection.exec("PRAGMA wall_checkpoint(TRUNCATE);");
    assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
}

std::shared_ptr<DeskphoneCache> DeskphoneCache::create(YAML::Node const &config)
{
    auto const filename = config["filename"].as<std::string>();
    std::chrono::minutes const expiry{std::max<uint32_t>(60, config["expiry"].as<uint32_t>())};
    return std::make_shared<DeskphoneCache>(filename, expiry);
}

/// Initializes an SQLite3 database with tables used to maintain the active deskphone details. If the table of active
/// deskphones already exists all deskphones in the database set to an expired state so that deskphone re-registration
/// can immediately occur.
void DeskphoneCache::initializeDatabase()
{
    syslog(LOG_DEBUG, "Initializing database cache");

    connection_pool_.setPrepSql("PRAGMA journal_mode=WAL; PRAGMA busy_timeout=3000;");

    auto connection = connection_pool_.getConnection();
    connection.exec(" CREATE TABLE IF NOT EXISTS endpoints ("
                    "aor TEXT NOT NULL,"
                    " endpoint TEXT NOT NULL,"
                    " ip_ver TEXT,"
                    " trans TEXT,"
                    " ip TEXT NOT NULL,"
                    " port INT,"
                    " expiry INT64,"
                    " PRIMARY KEY(aor, endpoint));");

    // Database may contain old AOR endpoints that haven't expired from the cache yet. Expire these endpoints so that
    // initial lamp updates work.
    connection.exec("UPDATE endpoints SET expiry = 0;");
}

/// Adds a deskphone to the table of active deskphones on the SIP network.
///
/// If the deskphone already exists in the table of active deskphones  the timestamp of that record is updated to the
/// current time.
///
/// This function will return \c true if the deskphone didn't exist in the table of active deskphones. Otherwise, this
/// function will return \c false indicating that the deskphone already existed in the table.
bool DeskphoneCache::addEndpoint(std::string const &aor, std::string const &endpoint)
{
    syslog(LOG_DEBUG, "DeskphoneCache::addEndpoint(\"%s\", \"%s\")", aor.c_str(), endpoint.c_str());

    // Query table for unexpired handset
    auto connection = connection_pool_.getConnection();
    auto check =
        connection.getStmt("SELECT EXISTS(SELECT 1 FROM endpoints WHERE aor = ? AND endpoint = ? AND expiry > ?);");
    check.bindText(1, aor);
    check.bindText(2, endpoint);
    auto const now = clock_t::now();
    check.bindInt64(3, now.time_since_epoch().count());
    auto const ret = check.execute();
    assert(ret == dbpool::PreparedStmt::ReturnCode::Row);
    auto const found_rec{check.getBool(0)};

    // Enqueue adding endpoint to the deskphone cache
    std::lock_guard const lock(batch_mut_);
    batch_.emplace_back(SQLAction::insert, aor, endpoint, (now + expiry_).time_since_epoch().count());
    batch_write_cv_.notify_one();

    return !found_rec;
}

/// This function removes the deskphone identified by \c aor and \c ip from the table of active deskphones.
void DeskphoneCache::deleteEndpoint(std::string const &aor, std::string const &ip)
{
    syslog(LOG_DEBUG, "DeskphoneCache::deleteEndpoint(\"%s\", \"%s\")", aor.c_str(), ip.c_str());

    // Enqueue expiring endpoint from the deskphone cache
    std::lock_guard const lock(batch_mut_);
    batch_.emplace_back(SQLAction::remove, aor, ip, 0);
    batch_write_cv_.notify_one();
}

void DeskphoneCache::forEachAOR(std::function<void(std::string const &)> const &lambda)
{
    auto connection = connection_pool_.getConnection();
    auto select_stmt = connection.getStmt("SELECT DISTINCT aor FROM endpoints WHERE expiry > ?;");
    auto const now = clock_t::now();
    select_stmt.bindInt64(1, now.time_since_epoch().count());
    while (select_stmt.execute() == dbpool::PreparedStmt::ReturnCode::Row) {
        lambda(select_stmt.getText(0));
    }
}

void DeskphoneCache::startWriteThread()
{
    batch_write_run_ = true;
    batch_write_thread_ = std::thread(&DeskphoneCache::writeThread, this);

    pthread_setname_np(batch_write_thread_.native_handle(), "db_writer");
}

void DeskphoneCache::stopWriteThread()
{
    batch_write_run_ = false;
    batch_write_cv_.notify_one();

    assert(batch_write_thread_.joinable());
    batch_write_thread_.join();
}

/// SQLite3 natively supports multiple simultaneous readers from a database but only supports one writer at any given
/// moment. Simultaneous writes can fail with a SQLITE_BUSY error, requiring additional code to handle such an event. In
/// order to circumvent this this object will handle all writes from a single thread. This should slightly improve
/// performance as writes are done in the background with the caveat being that any read immediately following the
/// enqueueing of a write may be out of date.
void DeskphoneCache::writeThread()
{
    syslog(LOG_DEBUG, "DeskphoneCache::writeThread() : Start thread");

    // Rather than recompiling prepared statements for every iteration of this service thread this function will compile
    // the statements a single time for the duration of this thread.
    auto connection = connection_pool_.getConnection();

    // Transactions are going to be batched for atomicity, improved data coherence, and faster performance.
    auto begin = connection.getStmt("BEGIN;");
    auto commit = connection.getStmt("COMMIT;");

    // Avoid deleting the record since that can be expensive and lead to a fragmented database file. Just expire the
    // record since the phone may show up at a later time.
    auto del_aor = connection.getStmt("UPDATE endpoints SET expiry = 0 WHERE aor = ? AND ip = ?;");
    auto add_aor = connection.getStmt(
        "INSERT INTO endpoints (aor, endpoint, expiry, ip_ver, trans, ip, port)"
        " VALUES (?, ?, ?, ?, ?, ?, ?)"
        " ON CONFLICT(aor, endpoint)"
        " DO UPDATE SET expiry = excluded.expiry, ip_ver = excluded.ip_ver, trans = excluded.trans, ip = excluded.ip, port = excluded.port;");

    // Lambda to bind encoded fields to record. The endpoint value coming from Asterisk is actually several fields
    // delimited by forward slashes. This lambda will split the endpoint up into its separate fields and bind them to
    // the insert statement.
    auto bind_fields = [&add_aor](int idx, std::string const &endpoint) mutable -> void {
        // Split endpoint into its separate fields
        auto const fields = endpoint | std::views::split('/') | std::views::transform([](auto &&r) {
                                return std::string_view(&*r.begin(), std::ranges::distance(r));
                            });

        // Bind fields to the insert statement
        for (auto const field : fields) {
            if (idx != 7) {
                add_aor.bindText(idx, field);
            }
            else {
                int port{};
                auto const result = std::from_chars(field.begin(), field.end(), port);
                if (result.ec == std::errc{}) {
                    add_aor.bindInt32(idx, port);
                }
                else {
                    add_aor.bindNull(idx);
                }
            }
            ++idx;
        }
    };

    // Working batch
    std::vector<HandsetData> batch;

    // Service loop
    while (batch_write_run_) {
        std::unique_lock lock(batch_mut_);
        batch_write_cv_.wait(lock, [this]() -> bool { return !batch_write_run_ || !batch_.empty(); });
        std::swap(batch_, batch);
        lock.unlock();

        // Begin transaction(s)
        auto ret = begin.execute();
        assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
        begin.reset();

        // Add/remove record(s) from database
        for (auto const &data : batch) {
            if (data.action == SQLAction::insert) {
                add_aor.bindText(1, data.aor);
                add_aor.bindText(2, data.endpoint);
                add_aor.bindInt64(3, data.expiry);
                bind_fields(4, data.endpoint);
                ret = add_aor.execute();
                assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
                add_aor.reset();
            }
            else {
                del_aor.bindText(1, data.aor);
                del_aor.bindText(2, data.endpoint);
                ret = del_aor.execute();
                assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
                del_aor.reset();
            }
        }

        // Commit transaction(s)
        ret = commit.execute();
        assert(ret == dbpool::PreparedStmt::ReturnCode::Done);
        commit.reset();

        batch.clear();
    }

    syslog(LOG_DEBUG, "DeskphoneCache::workThread() : Stop thread");
}
