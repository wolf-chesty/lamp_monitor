// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_NIGHT_BUTTON_HPP
#define BRIDGE_NIGHT_BUTTON_HPP

#include "asterisk/NightEventHandler.hpp"
#include "button_state/PhoneButton.hpp"
#include <c++ami/Connection.hpp>
#include <memory>
#include <string>
#include <string_view>

namespace bridge {

/// @class NightButton
/// @namespace bridge
///
/// @brief This object provides a night button bridge from the phone to the application.
///
/// This object is responsible for updating the night button device, changing the systems night state. After delegating
/// the state change to the Asterisk system it will return the expected button state to the user of this object.
class NightButton {
public:
    explicit NightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string device);
    virtual ~NightButton() = default;

    /// @brief Creates a phone HTTP night button object for the phone of type \c type.
    ///
    /// @param type Type of deskphone.
    /// @param button Application phone button state.
    /// @param conn Connection to Asterisk server.
    /// @param device Device on Asterisk server holding button state.
    ///
    /// @return Pointer to new HTTP night button object.
    static std::shared_ptr<NightButton> create(std::string_view type,
                                               std::shared_ptr<button_state::PhoneButton> const &button,
                                               std::shared_ptr<cpp_ami::Connection> const &conn,
                                               std::string const &device);

    /// @brief Updates the button state on the Asterisk server and returns the new phone screen state.
    ///
    /// @return Phone screen state.
    std::string pushButton();

    /// @brief Returns phone screen content type.
    ///
    /// @return Phone screen content type.
    virtual std::string getContentType() = 0;

protected:
    /// @brief Returns the phone button state compatible for display on phones.
    ///
    /// @param phone_button Application phone button state.
    /// @param button_on New phone button state.
    ///
    /// @return Expected state of the deskphone. String can be sent to the deskphone to update its state.
    virtual std::string pushButton(std::shared_ptr<button_state::PhoneButton> const &phone_button, bool button_on) = 0;

private:
    std::shared_ptr<button_state::PhoneButton> button_; ///< Pointer to the application button state.
    std::shared_ptr<cpp_ami::Connection> io_conn_;      ///< Connection to the Asterisk AMI server.
    std::string device_;                                ///< Asterisk device that represents the systems night state.
};

} // namespace bridge

#endif