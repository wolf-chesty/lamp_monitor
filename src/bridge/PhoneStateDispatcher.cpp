// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/PhoneStateDispatcher.hpp"

#include <cassert>
#include <syslog.h>

using namespace bridge;

PhoneStateDispatcher::PhoneStateDispatcher(std::shared_ptr<cpp_ami::Connection> io_conn,
                                           std::shared_ptr<DeskphoneCache> deskphone_cache)
    : io_conn_(std::move(io_conn))
    , deskphone_cache_(std::move(deskphone_cache))
{
    assert(io_conn_);
    assert(deskphone_cache_);

    syslog(LOG_DEBUG, "PhoneStateDispatcher::PhoneStateDispatcher()");

    messages_.reserve(128);

    startDispatchThread();
}

PhoneStateDispatcher::~PhoneStateDispatcher()
{
    syslog(LOG_DEBUG, "PhoneStateDispatcher::~PhoneStateDispatcher()");

    stopDispatchThread();
}

void PhoneStateDispatcher::dispatch(std::vector<std::string> aors, cpp_ami::action::PJSIPNotify action)
{
    syslog(LOG_DEBUG, "PhoneStateDispatcher::dispatch(\"%s\")", action.toString().c_str());

    std::lock_guard const lock(messages_mut_);
    messages_.emplace_back(std::move(aors), std::move(action));
    messages_cv_.notify_one();
}

void PhoneStateDispatcher::startDispatchThread()
{
    work_thread_run_ = true;
    work_thread_ = std::thread(&PhoneStateDispatcher::dispatchThread, this);

    pthread_setname_np(work_thread_.native_handle(), "state_dispatch");
}

void PhoneStateDispatcher::stopDispatchThread()
{
    work_thread_run_ = false;
    messages_cv_.notify_one();

    assert(work_thread_.joinable());
    work_thread_.join();
}

void PhoneStateDispatcher::dispatchThread()
{
    syslog(LOG_DEBUG, "PhoneStateDispatcher::dispatchThread() : Start thread");

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

        syslog(LOG_DEBUG, "PhoneStateDispatcher::workThread() : Waking dispatch thread");

        for (auto &[aors, action] : messages) {
            for (auto const &aor : aors) {
                action["Endpoint"] = aor;
                io_conn_->asyncInvoke(action);
            }
        }
        messages.clear();
    }

    syslog(LOG_DEBUG, "PhoneStateDispatcher::workThread() : Stop thread");
}
