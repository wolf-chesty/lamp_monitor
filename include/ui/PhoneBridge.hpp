// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_PHONE_BRIDGE_HPP
#define UI_PHONE_BRIDGE_HPP

#include "cache/DeskphoneCache.hpp"
#include <atomic>
#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/Connection.hpp>
#include <condition_variable>
#include <string>
#include <thread>
#include <vector>

namespace ui {

/// @class PhoneBridge
/// @namespace ui
///
/// @brief This class will push the application UI phone state to the actual hardware deskphones on the network,
///        providing a software to hardware bridge.
class PhoneBridge {
public:
    explicit PhoneBridge(std::shared_ptr<cpp_ami::Connection> io_conn, std::shared_ptr<DeskphoneCache> deskphone_cache);
    ~PhoneBridge();

    /// @brief Add notification message to send to deskphones to queue for dispatch.
    ///
    /// @param phone_id Phone ID to send the notification action to.
    /// @param action PJSIP notification action to send.
    void dispatch(std::string const &phone_id, cpp_ami::action::PJSIPNotify action);

private:
    void startWorkThread();
    void stopWorkThread();
    void workThread();

    std::shared_ptr<cpp_ami::Connection> io_conn_;
    std::shared_ptr<DeskphoneCache> deskphone_cache_;
    std::vector<std::pair<std::string, cpp_ami::action::PJSIPNotify>> messages_;
    std::mutex messages_mut_;
    std::condition_variable messages_cv_;
    std::thread work_thread_;
    std::atomic<bool> work_thread_run_;
};

} // namespace ui

#endif
