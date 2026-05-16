// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_YEALINK_NIGHT_BUTTON_HPP
#define BRIDGE_YEALINK_NIGHT_BUTTON_HPP

#include "bridge/NightButton.hpp"

namespace bridge::yealink {

/// @class NightButton
/// @namespace bridge::yealink
///
/// @brief This object provides a night button bridge from the phone to the application.
///
/// This object is responsible for updating the night button device, changing the systems night state. After delegating
/// the state change to the Asterisk system it will return the expected button state to the user of this object.
class NightButton : public bridge::NightButton {
public:
    explicit NightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string device);
    ~NightButton() override = default;

    /// @brief Returns the phone button state compatible for display on Yealink phones.
    ///
    /// @param phone_button Application phone button state.
    /// @param button_on New phone button state.
    ///
    /// @return Expected state of the deskphone. String can be sent to the deskphone to update its state.
    std::string pushButton(std::shared_ptr<button_state::PhoneButton> const &phone_button, bool button_on) override;

    /// @brief Returns content type for HTTP text created by this object.
    ///
    /// @return HTTP content type.
    std::string getContentType() override;
};

} // namespace bridge::yealink

#endif
