// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_STATE_PHONE_BUTTON_HPP
#define LAMP_STATE_PHONE_BUTTON_HPP

#include <atomic>
#include <memory>

namespace lamp_state {

class LampField;

/// @class PhoneButton
/// @namespace lamp_state
///
/// @brief Represents the button state on a deskphone.
class PhoneButton {
    friend class LampField;

public:
    enum class Color { Red, Green, Blue };

public:
    explicit PhoneButton(uint16_t const button_id, Color const color, bool const flash, bool const is_critical = false,
                         bool const button_on = false);
    explicit PhoneButton(std::shared_ptr<LampField> lamp_field, uint16_t const button_id, Color const color,
                         bool const flash, bool const is_critical = false, bool const button_on = false);
    ~PhoneButton() = default;

    /// @brief Returns the button ID on the deskphone.
    ///
    /// @return Button ID.
    uint16_t buttonID() const;

    /// @brief Returns the button color.
    ///
    /// @return Button color.
    Color color() const;

    /// @brief Returns \c true if the button should flash.
    ///
    /// @return \c true if button flashes when in the on state.
    bool flash() const;

    /// @brief Returns \c true if the button is in the on state.
    ///
    /// @return \c true if button is in the on state.
    bool isOn() const;

    /// @brief Sets the button state.
    ///
    /// @param on \c true if button state should be set to on.
    ///
    /// @return Previous button state.
    bool setOn(bool const on);

    /// @brief Returns \c true if the button is critical and updates should occur.
    ///
    /// @return \c true if button requires phone screen update.
    bool isCritical() const;

    /// @brief Sets the buttons critical state.
    ///
    /// @return Previous critical flag for the button.
    bool setIsCritical(bool const update_required);

    /// @brief Sets the buttons on and critical state.
    ///
    /// @param is_on Flag indicating if the button should be in the on state.
    /// @param is_critical Flag indicating if the button should be in the critical state.
    ///
    /// @return \c true if phone state was updated.
    bool setState(bool const is_on, bool const is_critical);

protected:
    /// @brief Updates the lamp field with the new button state.
    void updateLampField();

private:
    std::shared_ptr<LampField> lamp_field_; ///< Pointer to lamp field that monitors this button.
    uint16_t button_id_;                    ///< Button ID in lamp field.
    Color color_;                           ///< Button color for deskphones that support color displays.
    bool flash_{false};                     ///< Button should flash.
    std::atomic<bool> is_critical_{false};  ///< Flag indicating phone screen should be updated.
    std::atomic<bool> button_on_{false};    ///< Current button state.
};

} // namespace lamp_state

#endif
