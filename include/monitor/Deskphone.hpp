// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_YEALINK_LAMP_FIELD_HPP
#define MONITOR_YEALINK_LAMP_FIELD_HPP

#include "button_state/LampFieldObserver.hpp"

#include "DeskphoneCache.hpp"
#include "monitor/PhoneUIAdapter.hpp"
#include <atomic>
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <condition_variable>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>

namespace monitor {

/// @class Deskphone
/// @namespace monitor
///
/// @brief Monitors for deskphone registration events coming from the Asterisk server.
///
/// Objects of this class will monitor Asterisk AMI events for the occurrence ov PJSIP registrations. Upon receiving a
/// new PJSIP registration the object will cache the SIP endpoint and notify the endpoint of the current button state.
///
/// Upon button state change this object will notify all deskphone endpoints of the new button state.
///
/// Deskphone content rendering is handled by \c PhoneUIAdapter objects. In order to handle phones of varying types a
/// \c PhoneUIAdapter for the phone should be registered to this object.
class Deskphone : public button_state::LampFieldObserver {
public:
    explicit Deskphone(std::shared_ptr<DeskphoneCache> deskphone_cache, std::shared_ptr<cpp_ami::Connection> io_conn);
    ~Deskphone() override;

    /// @brief Registers a new phone UI adapter to this object.
    ///
    /// @param ui_adapter Phone UI adapter to register with this object.
    void registerPhoneAdapter(std::shared_ptr<PhoneUIAdapter> const &ui_adapter);

    /// @brief Unregisters a phone UI adapter from this object.
    ///
    /// @param ui_adatper Phone UI adapter to unregister from this object.
    void unregisterPhoneAdapter(std::shared_ptr<PhoneUIAdapter> const &ui_adapter);

    /// @brief Invalidates the states of buttons being monitored by this object.
    ///
    /// @param buttons Collection of buttons that have had their state updated.
    void invalidate(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons) override;

protected:
    /// @brief Returns collection of phone UI adapters for this deskphone.
    ///
    /// @return Pointer to a deskphone UI adapter.
    std::shared_ptr<PhoneUIAdapter> getPhoneAdapter();

private:
    /// @brief Handles AMI events coming from the Asterisk AMI server.
    ///
    /// @param event Event sent by the Asterisk AMI server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    void publishPhoneState(std::shared_ptr<PhoneUIAdapter> const &phone_adapter);
    void publishPhoneState(std::string const &aor, std::shared_ptr<PhoneUIAdapter> const &phone_adapter);

    void startWorkThread();
    void stopWorkThread();
    void workThread();

    std::shared_ptr<DeskphoneCache> deskphone_cache_;           ///< Deskphone cache.
    std::shared_ptr<cpp_ami::Connection> io_conn_;              ///< Asterisk AMI connection.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< AMI callback handler ID.
    std::shared_ptr<PhoneUIAdapter> deskphone_adapter_;         ///< Phone renderers.
    std::shared_mutex deskphone_adapter_mut_;                   ///< Mutex on phone renderers.
    std::atomic<bool> button_state_valid_{true};
    std::thread work_thread_;
    std::atomic<bool> work_thread_run_{};
    std::condition_variable work_thread_cv_{};
};

} // namespace monitor

#endif
