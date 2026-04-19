// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_HTTP_NIGHT_BUTTON_HPP
#define XML_YEALINK_HTTP_NIGHT_BUTTON_HPP

#include "ui/HttpNightButton.hpp"
#include <button_state/PhoneButton.hpp>
#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace xml::yealink {

/// @class HTTPNightButton
/// @namespace xml::yealink
///
/// @brief This object provides an HTTP facing "night button".
///
/// This object is responsible for updating the night button device, changing the systems night state. After delegating
/// the state change to the Asterisk system it will return the expected button state to the user of this object.
class HTTPNightButton : public ui::HTTPNightButton {
public:
    explicit HTTPNightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                             std::shared_ptr<cpp_ami::Connection> io_conn, std::string device);
    ~HTTPNightButton() = default;

    /// @brief Pushes the night "button", changing the Asterisk night state.
    ///
    /// @return Expected state of the deskphone. String can be sent to the deskphone to update its state.
    std::string httpPushButton() override;

    std::string getContentType() override;

private:
    std::shared_ptr<button_state::PhoneButton> button_; ///<  Pointer to the application button state.
    std::shared_ptr<cpp_ami::Connection> io_conn_;      ///< Connection to the Asterisk AMI server.
    std::string device_;                                ///< Asterisk device that represents the systems night state.
};

} // namespace xml::yealink

#endif
