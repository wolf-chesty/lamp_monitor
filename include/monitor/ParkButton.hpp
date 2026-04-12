// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_PARK_BUTTON_HPP
#define MONITOR_PARK_BUTTON_HPP

#include "button_state/PhoneButton.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <string>
#include <unordered_set>

namespace monitor {

/// @class ParkButton
/// @namespace monitor
///
/// @brief Monitors parked calls on the Asterisk server updating the park buttons state on arrival/departure of parked
///        calls.
class ParkButton {
public:
    explicit ParkButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                        std::shared_ptr<cpp_ami::Connection> io_conn);
    ~ParkButton();

private:
    /// @brief Invoked whenever there is a new AMI event from Asterisk.
    ///
    /// @param event Event that occurred on the Asterisk server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    /// @brief Updates the phone button state based on if there are parked calls present.
    ///
    /// @param parked_calls_present \c true if parked calls are present.
    void updateButtonState(bool const parked_calls_present);

    std::shared_ptr<button_state::PhoneButton> phone_button_;   ///< Pointer to phone button state object.
    std::shared_ptr<cpp_ami::Connection> io_conn_;              ///< Pointer to Asterisk AMI connection.
    std::unordered_set<std::string> parked_extens_;             ///< Collection of active parked extensions.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< Event callback ID.
};

} // namespace monitor

#endif