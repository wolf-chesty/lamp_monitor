// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_PARK_BUTTON_HPP
#define BRIDGE_PARK_BUTTON_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace bridge {

/// @class ParkButton
/// @namespace bridge
///
/// @brief Objects of this type provide a park button bridge from the phone to the application.
///
/// This object is responsible for gathering a list of parked calls on the Asterisk system and presenting a call park
/// menu on the phone display.
class ParkButton {
public:
    ParkButton() = default;
    virtual ~ParkButton() = default;

    /// @brief Creates a phone park button object for the phone of type \c type.
    ///
    /// @param type Type of deskphone.
    /// @param conn Connection to Asterisk server.
    /// @param parking_lot Parking long identifier.
    /// @param parked_call_info_uri URI for the parked call list.
    static std::shared_ptr<ParkButton> create(std::string_view type, std::shared_ptr<cpp_ami::Connection> const &conn,
                                              std::string const &parking_lot, std::string const &parked_call_info_uri);

    /// @brief Deskphone park button presses will be routed to this function.
    ///
    /// @return Parked call menu.
    virtual std::string pushButton() const = 0;

    /// @brief Deskphone parked call info button presses will be routed to this function.
    ///
    /// @param exten Parked extension.
    ///
    /// @return Parked call info screen.
    virtual std::string pushButton(std::string const &exten) const = 0;

    /// @brief Displays an error screen on the phone screen with \c title and text body \c text.
    ///
    /// @param title Title of phone screen.
    /// @param text Text to display on phone screen.
    ///
    /// @return Phone screen.
    virtual std::string displayErrorMessage(std::string const &title, std::string const &text) const = 0;

    /// @brief Returns the HTTP content-type for the phone screen.
    ///
    /// @return HTTP content-type for phone screen data.
    virtual std::string getContentType() const = 0;
};

} // namespace bridge

#endif
