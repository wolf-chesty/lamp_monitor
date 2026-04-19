// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ui/PhoneEventDispatcher.hpp"

#include <cassert>
#include <syslog.h>

using namespace ui;

PhoneEventDispatcher::PhoneEventDispatcher(std::shared_ptr<cpp_ami::Connection> io_conn,
                                           std::shared_ptr<DeskphoneCache> deskphone_cache)
    : io_conn_(std::move(io_conn))
    , deskphone_cache_(std::move(deskphone_cache))
{
    assert(io_conn_);
    assert(deskphone_cache_);

    syslog(LOG_DEBUG, "PhonePJSIPNotifyBridge::PhonePJSIPNotifyBridge()");

    messages_.reserve(128);

    startDispatchThread();
}

PhoneEventDispatcher::~PhoneEventDispatcher()
{
    syslog(LOG_DEBUG, "PhonePJSIPNotifyBridge::~PhonePJSIPNotifyBridge()");

    stopDispatchThread();
}

void PhoneEventDispatcher::dispatch(cpp_ami::action::PJSIPNotify const &action)
{
    syslog(LOG_DEBUG, "PhonePJSIPNotifyBridge::dispatch(\"%s\")", action.toString().c_str());

    std::lock_guard const lock(messages_mut_);
    messages_.emplace_back(action);
    messages_cv_.notify_one();
}

void PhoneEventDispatcher::startDispatchThread()
{
    work_thread_run_ = true;
    work_thread_ = std::thread(&PhoneEventDispatcher::dispatchThread, this);

    pthread_setname_np(work_thread_.native_handle(), "phone_bridge");
}

void PhoneEventDispatcher::stopDispatchThread()
{
    work_thread_run_ = false;
    messages_cv_.notify_one();

    assert(work_thread_.joinable());
    work_thread_.join();
}

void PhoneEventDispatcher::dispatchThread()
{
    syslog(LOG_DEBUG, "PhonePJSIPNotifyBridge::workThread() : Start thread");

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

        for (auto &action : messages) {
            deskphone_cache_->forEachAOR([this, action](std::string_view aor) mutable -> void {
                action["Endpoint"] = aor;
                io_conn_->asyncInvoke(action);
            });
        }
        messages.clear();
    }

    syslog(LOG_DEBUG, "PhonePJSIPNotifyBridge::workThread() : Stop thread");
}
