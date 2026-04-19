// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_NIGHT_EVENT_HANDLER_HPP
#define AST_BRIDGE_NIGHT_EVENT_HANDLER_HPP

#include "asterisk/EventHandler.hpp"

#include "button_state/PhoneButton.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

namespace asterisk {

/// @class NightEventHandler
/// @namespace asterisk
///
/// @brief Monitors the Asterisk server for night state change.
///
/// This object acts as an Asterisk to application state bridge. This object will inspect Asterisk AMI event messages
/// for state changes in the night device. Once a night device change is detected, this object will delegate the new
/// state to the application state button for this event.
class NightEventHandler : public EventHandler {
public:
    explicit NightEventHandler(std::weak_ptr<button_state::PhoneButton> phone_button,
                               std::shared_ptr<cpp_ami::Connection> io_conn, std::string night_exten,
                               std::string context, std::string device);
    ~NightEventHandler() override;

    /// @brief Creates an object of this type using the parameters from \c config.
    ///
    /// @param config Configuration parameters for object.
    /// @param phone_button Application button for application state change delegation.
    /// @param conn Asterisk AMI connection to monitor for Asterisk events.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<NightEventHandler> create(YAML::Node const &config,
                                                     std::weak_ptr<button_state::PhoneButton> const &phone_button,
                                                     std::shared_ptr<cpp_ami::Connection> const &conn);

    /// @brief Returns type for this object.
    ///
    /// @return Event type.
    EventType getType() const override;

    /// @brief Returns the device that represents the night state on the Asterisk system.
    ///
    /// @return Name of the Asterisk device that keeps the Asterisk night state.
    std::string getDevice();

private:
    /// @brief Function that processes Asterisk AMI events.
    ///
    /// @param event Event that occurred on the Asterisk server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::weak_ptr<button_state::PhoneButton> phone_button_;     ///< Pointer to phone button state.
    std::string night_exten_;                                   ///< Extension of the night button.
    std::string context_;                                       ///< Context that the night extension is a member of.
    std::string device_;                                        ///< Device name of the night button.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< Asterisk AMI callback ID.
};

} // namespace asterisk

#endif
