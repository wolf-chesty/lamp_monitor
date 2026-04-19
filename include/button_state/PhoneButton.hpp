// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BUTTON_STATE_PHONE_BUTTON_HPP
#define BUTTON_STATE_PHONE_BUTTON_HPP

#include <atomic>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace button_state {

class ButtonPlan;

/// @class PhoneButton
/// @namespace button_state
///
/// @brief This object represents a button state of a deskphone.
///
/// This object is the application representation of a deskphone button. Each button belongs to a lamp field, and is
/// responsible for reporting any button state change to the lamp field for further processing.
class PhoneButton {
public:
    /// @enum Color
    ///
    /// @brief Represents the different button colors.
    enum class Color { Red, Green, Blue };

    /// @enum FlashMode
    ///
    /// @brief Represents the flash mode of an active button.
    enum class FlashMode { Solid, Slow, Fast };

public:
    explicit PhoneButton(std::weak_ptr<ButtonPlan> button_plan, uint16_t const button_id, Color const color,
                         FlashMode const flash_mode, bool const critical_when_on, bool const critical_when_off,
                         bool const button_on = false);
    ~PhoneButton() = default;

    /// @brief Creates a new object using configuration parameters from \c config.
    ///
    /// @param config Configuration parameters.
    /// @param button_plan Pointer to button plan that owns this object.
    ///
    /// @return Pointer to new phone button state object.
    static std::shared_ptr<PhoneButton> create(YAML::Node const &config, std::weak_ptr<ButtonPlan> const &button_plan);

    /// @brief Clones this object.
    ///
    /// @return Pointer to clone of this object.
    std::shared_ptr<PhoneButton> clone();

    /// @brief Returns the button ID on the deskphone.
    ///
    /// @return Button ID.
    uint16_t getButtonID() const;

    /// @brief Returns the label for the button.
    ///
    /// @return Button label.
    std::string getLabel() const;

    /// @brief Returns the button color.
    ///
    /// @return Button color.
    Color getColor() const;

    /// @brief Returns flash mode of the button when it is active.
    ///
    /// @return \c FlashMode for the button.
    FlashMode getFlashMode() const;

    /// @brief Returns \c true if the button is in the on state.
    ///
    /// @return \c true if button is in the on state.
    bool isOn() const;

    /// @brief Sets the button state.
    ///
    /// @param on \c true if button state should be set to on.
    void setOn(bool const on);

    /// @brief Returns \c true if the button is critical and updates should occur.
    ///
    /// @return \c true if button requires phone screen update.
    bool isCritical() const;

protected:
    /// @brief Updates the lamp field with the new button state.
    void triggerLampFieldUpdate();

private:
    /// @brief Converts YAML color type string to an enum.
    ///
    /// @param color_type YAML color type string.
    ///
    /// @return Enum color type.
    static Color toColor(std::string_view color_type);

    /// @brief Converts YAML flash mode type string to an enum.
    ///
    /// @param flash_mode YAML flash mode type string.
    ///    /// @return Enum flash type.
    static FlashMode toFlashMode(std::string_view flash_mode);

    std::weak_ptr<ButtonPlan> button_plan_;  ///< Pointer to lamp field that monitors this button.
    uint16_t button_id_{};                   ///< Button ID in lamp field.
    std::string label_;                      ///< Button label.
    Color color_{Color::Red};                ///< Button color for deskphones that support color displays.
    FlashMode flash_mode_{FlashMode::Solid}; ///< Flash mode for on button.
    bool critical_when_on_{false};           ///< Flag indicating message is critical when button is on.
    bool critical_when_off_{false};          ///< Flag indicating message is critical when button is off.
    std::atomic<bool> button_on_{false};     ///< Current button state.
};

} // namespace button_state

#endif
