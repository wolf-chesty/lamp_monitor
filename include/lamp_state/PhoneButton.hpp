// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_STATE_PHONE_BUTTON_HPP
#define LAMP_STATE_PHONE_BUTTON_HPP

#include "lamp_state/LampField.hpp"
#include <atomic>
#include <memory>

namespace lamp_state {

/// @class PhoneButton
/// @namespace lamp_state
///
/// @brief Represents the button state on a deskphone.
class PhoneButton {
public:
    enum class Color { Red, Green, Blue };

public:
    explicit PhoneButton(std::shared_ptr<LampField> lamp_field, uint16_t button_id, Color color, bool flash);
    ~PhoneButton() = default;

    /// @brief Returns the button ID on the deskphone.
    ///
    /// @return Button ID.
    uint16_t buttonID() const;

    Color color() const;

    bool flash() const;

    bool isOn() const;

    bool setOn(bool const on);

private:
    std::shared_ptr<LampField> lamp_field_;
    uint16_t button_id_;
    Color color_;
    bool flash_{false};
    std::atomic<bool> button_on_{false};
};

} // namespace lamp_state

#endif
