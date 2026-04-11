// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_FIELD_MONITOR_HPP
#define LAMP_FIELD_MONITOR_HPP

#include "DeskphoneCache.hpp"
#include "LampFieldState.hpp"
#include "LampMonitor.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

/// @class LampFieldMonitor
///
/// @brief Updates the lamp state of the deskphones.
///
/// This object is responsible for monitoring the registration of deskphones to the Asterisk server. When new deskphones
/// are registered to the Asterisk server this object will update the new deskphones button states.
///
/// This object also maintains a collection of lamp objects. These lamp objects can invalidate their state triggering
/// this object to notify all deskphones registered with the Asterisk server of the new button state.
class LampFieldMonitor : public std::enable_shared_from_this<LampFieldMonitor> {
public:
    explicit LampFieldMonitor(std::unique_ptr<DeskphoneCache> handset_cache,
                              std::shared_ptr<cpp_ami::Connection> io_conn);
    virtual ~LampFieldMonitor();

    void addLamp(std::shared_ptr<LampMonitor> const &lamp);
    void addLamps(std::list<std::shared_ptr<LampMonitor>> const &lamps);

    std::shared_ptr<LampFieldState> getCachedButtonState();
    static std::shared_ptr<LampFieldState> getButtonState(std::vector<std::shared_ptr<LampMonitor>> const &lamps,
                                                          bool beep = false);

    void invalidateAOR(std::string const &aor, std::string const &ip);
    void invalidateButtonState();

private:
    /// @brief AMI event handler for this object.
    ///
    /// @param event AMI event to process.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::shared_ptr<LampFieldState> getButtonState();
    std::shared_ptr<LampFieldState> cacheButtonState(std::shared_ptr<LampFieldState> const &state);

    void publishButtonState(std::string const &aor, std::string const &button_state_xml) const;
    void publishButtonState(std::string const &button_state_xml) const;

    void startWorkThread();
    void stopWorkThread();
    void workThread();

    std::shared_ptr<cpp_ami::Connection> io_conn_;              ///< Pointer to AMI connection.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< AMI callback handler ID.
    std::vector<std::shared_ptr<LampMonitor>> lamps_; ///< Collection of objects that monitor phone lamp state.
    std::mutex lamps_mut_;                            ///< Mutex on \c LampMonitor collection.
    std::thread button_state_thread_;                 ///< Handle to thread that updates the phone states.
    std::atomic<bool> button_state_thread_run_{};     ///< Flag to stop phone update thread.
    std::atomic<bool> button_state_valid_{true};      ///< Flag indicating lamp field validity.
    std::condition_variable button_state_cv_;         ///< Condition variable to trigger phone state update.
    std::unique_ptr<DeskphoneCache> handset_cache_;     ///< Cache of active handsets on the system.
    std::shared_ptr<LampFieldState> cached_state_;    ///< Cached lamp field state.
    std::mutex cached_state_mut_;                     ///< Mutex protecting cached lamp field state.
};

#endif