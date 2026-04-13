// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_NIGHT_BUTTON_HPP
#define AST_BRIDGE_NIGHT_BUTTON_HPP

#include "button_state/PhoneButton.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>

namespace ast_bridge {

/// @class NightButton
/// @namespace ast_bridge
///
/// @brief Monitors the Asterisk server for night state change.
///
/// This object acts as an Asterisk to application state bridge. This object will inspect Asterisk AMI event messages
/// for state changes in the night device. Once a night device change is detected, this object will delegate the new
/// state to the application state button for this event.
class NightButton {
public:
    explicit NightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string night_exten, std::string context,
                         std::string device);
    virtual ~NightButton();

protected:
    /// @brief Returns a pointer to the Asterisk AMI connection object.
    ///
    /// @return Pointer to the objects Asterisk AMI connection object.
    std::shared_ptr<cpp_ami::Connection> getAMIConnection();

    /// @brief Returns the device that holds the Asterisk night button state.
    ///
    /// @return Name of the Asterisk device holding the night button state.
    std::string const &getDevice();

    /// @brief Returns pointer to the phone button state object.
    ///
    /// @return Pointer to the phone button state.
    std::shared_ptr<button_state::PhoneButton> getPhoneButton();

private:
    /// @brief Function that processes Asterisk AMI events.
    ///
    /// @param event Event that occurred on the Asterisk server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::shared_ptr<button_state::PhoneButton> phone_button_;   ///< Pointer to phone button state.
    std::shared_ptr<cpp_ami::Connection> io_conn_;              ///< Pointer to Asterisk AMI object.
    std::string night_exten_;                                   ///< Extension of the night button.
    std::string context_;                                       ///< Context that the night extension is a member of.
    std::string device_;                                        ///< Device name of the night button.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< Asterisk AMI callback ID.
};

} // namespace ast_bridge

#endif
