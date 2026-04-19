// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_PARK_EVENT_HANDLER_HPP
#define AST_BRIDGE_PARK_EVENT_HANDLER_HPP

#include "asterisk/EventHandler.hpp"

#include "button_state/PhoneButton.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <string>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

namespace asterisk {

/// @class ParkEventHandler
/// @namespace asterisk
///
/// @brief Monitors the Asterisk server for parked call events.
///
/// This object acts as an Asterisk to application state bridge. This object will inspect Asterisk AMI event messages
/// for any state changes in the Asterisk servers parked call space. As calls leave or enter the parked call space, this
/// object will delegate the state change to the application state button for parking.
class ParkEventHandler : public EventHandler {
public:
    explicit ParkEventHandler(std::weak_ptr<button_state::PhoneButton> phone_button,
                              std::shared_ptr<cpp_ami::Connection> io_conn, std::string parking_lot);
    ~ParkEventHandler() override;

    static std::shared_ptr<ParkEventHandler> create(YAML::Node const &config,
                                                    std::weak_ptr<button_state::PhoneButton> const &phone_button,
                                                    std::shared_ptr<cpp_ami::Connection> const &conn);

    EventType getType() const override;

    std::string getParkingLot() const;

private:
    /// @brief Invoked whenever there is a new AMI event from Asterisk.
    ///
    /// @param event Event that occurred on the Asterisk server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    /// @brief Updates the phone button state based on if there are parked calls present.
    ///
    /// @param parked_calls_present \c true if parked calls are present.
    void updateButtonState(bool const parked_calls_present);

    std::string parking_lot_;                                   ///< Name of parking lot being monitored.
    std::weak_ptr<button_state::PhoneButton> phone_button_;     ///< Pointer to phone button state object.
    std::unordered_set<std::string> parked_extens_;             ///< Collection of active parked extensions.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< Event callback ID.
};

} // namespace asterisk

#endif