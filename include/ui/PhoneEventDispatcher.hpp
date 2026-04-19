// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_ACTION_DISPATCHER_HPP
#define UI_ACTION_DISPATCHER_HPP

#include "cache/DeskphoneCache.hpp"
#include <atomic>
#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/Connection.hpp>
#include <condition_variable>
#include <string>
#include <thread>
#include <vector>

namespace ui {

/// @class PhoneEventDispatcher
/// @namespace ui
///
/// @brief This class will push the application UI phone state to the actual hardware deskphones on the network,
///        providing a software to hardware bridge.
class PhoneEventDispatcher {
public:
    explicit PhoneEventDispatcher(std::shared_ptr<cpp_ami::Connection> io_conn,
                                  std::shared_ptr<DeskphoneCache> deskphone_cache);
    ~PhoneEventDispatcher();

    /// @brief Add notification message to send to deskphones to queue for dispatch.
    ///
    /// @param action PJSIP notification action to send.
    void dispatch(cpp_ami::action::PJSIPNotify const &action);

private:
    /// @brief Starts the PJSIP notification dispatch thread.
    void startWorkThread();

    /// @brief Stops the PJSIP notification dispatch thread.
    void stopWorkThread();

    /// @brief Thread that dispatches PJSIP notifications in the background.
    void workThread();

    std::shared_ptr<cpp_ami::Connection> io_conn_;       ///< Connection to Asterisk AMI server.
    std::shared_ptr<DeskphoneCache> deskphone_cache_;    ///< Cache of deskphones.
    std::vector<cpp_ami::action::PJSIPNotify> messages_; ///< PJSIP notifications to dispatch.
    std::mutex messages_mut_;                            ///< Mutex on message collection.
    std::condition_variable messages_cv_;                ///< Condition variable to wake thread.
    std::thread work_thread_;                            ///< Handle to work thread.
    std::atomic<bool> work_thread_run_;                  ///< Flag to start/stop work thread.
};

} // namespace ui

#endif
