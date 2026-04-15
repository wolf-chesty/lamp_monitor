// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ui/PhoneBridge.hpp"

#include <cassert>
#include <syslog.h>

using namespace ui;

PhoneBridge::PhoneBridge(std::shared_ptr<cpp_ami::Connection> io_conn, std::shared_ptr<DeskphoneCache> deskphone_cache)
    : io_conn_(std::move(io_conn))
    , deskphone_cache_(std::move(deskphone_cache))
{
    assert(io_conn_);
    assert(deskphone_cache_);

    syslog(LOG_DEBUG, "PhoneBridge::PhoneBridge()");

    messages_.reserve(128);

    startWorkThread();
}

PhoneBridge::~PhoneBridge()
{
    syslog(LOG_DEBUG, "PhoneBridge::~PhoneBridge()");

    stopWorkThread();
}

void PhoneBridge::dispatch(std::string const &phone_id, cpp_ami::action::PJSIPNotify action)
{
    syslog(LOG_DEBUG, "PhoneBridge::dispatch(\"%s\", \"%s\")",phone_id.c_str(), action.toString().c_str());

    std::lock_guard const lock(messages_mut_);
    messages_.emplace_back(phone_id, std::move(action));
    messages_cv_.notify_one();
}

void PhoneBridge::startWorkThread()
{
    work_thread_run_ = true;
    work_thread_ = std::thread(&PhoneBridge::workThread, this);

    pthread_setname_np(work_thread_.native_handle(), "phone_bridge");
}

void PhoneBridge::stopWorkThread()
{
    work_thread_run_ = false;
    messages_cv_.notify_one();

    assert(work_thread_.joinable());
    work_thread_.join();
}

void PhoneBridge::workThread()
{
    syslog(LOG_DEBUG, "PhoneBridge::workThread() : Start thread");

    decltype(messages_) messages;
    messages.reserve(messages_.capacity());

    while (work_thread_run_) {
        std::unique_lock lock(messages_mut_);
        messages_cv_.wait(lock, [this]() -> bool { return !work_thread_run_ || !messages_.empty(); });
        std::swap(messages, messages_);
        lock.unlock();

        if (!work_thread_run_) {
            break;
        }

        for (auto const &itr : messages) {
            deskphone_cache_->forEachAOR([this, action = itr.second](std::string_view aor) mutable -> void {
                action["Endpoint"] = aor;
                io_conn_->asyncInvoke(action);
            });
        }
        messages.clear();
    }

    syslog(LOG_DEBUG, "PhoneBridge::workThread() : Stop thread");
}
