// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BUTTON_STATE_LAMP_FIELD_HPP
#define BUTTON_STATE_LAMP_FIELD_HPP

#include "button_state/PhoneButton.hpp"
#include "ui/PhoneBridge.hpp"
#include "ui/PhoneUi.hpp"
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace button_state {

/// @class LampField
/// @namespace button_state
///
/// @brief This object represents the current lamp field state of a deskphone.
///
/// This object is the applications representation of a deskphones lamp field. As such, each deskphone type will need to
/// have a lamp field object created for that particular device. If deskphones share a button layout, then perhaps lamp
/// field objects can be shared between deskphones.
///
/// This object also has a list of phone UI objects that are responsible for rendering button state of the physical
/// hardware deskphones. As button states occur this object will use this collection of phone UI objects to render the
/// current deskphone button state and send that information to each phone on the network.
class LampField : public std::enable_shared_from_this<LampField> {
    friend class PhoneButton;

public:
    explicit LampField(std::shared_ptr<ui::PhoneBridge> phone_bridge);
    virtual ~LampField() = default;

    /// @brief Creates a new phone button that is monitored by this object.
    ///
    /// @param button_id Button ID of new button.
    /// @param color Color of new button.
    /// @param flash \c true if new button should flash when the button is in the on state.
    /// @param critical \c true if button is a critical button.
    /// @param on \c true if  new button should already be in the on state.
    ///
    /// @return Pointer to a phone button.
    std::shared_ptr<PhoneButton> getButton(uint16_t const button_id, PhoneButton::Color const color, bool const flash,
                                           bool const critical = false, bool const on = false);

    /// @brief Removes button with button id of \c button_id from collection of buttons being monitored.
    ///
    /// @param button_id Button ID to remove from collection of monitored buttons.
    void removeButton(uint16_t const button_id);

    /// @brief Returns collection of buttons.
    ///
    /// @return Collection of button objects monitored by this object.
    std::vector<std::shared_ptr<PhoneButton>> getButtons();

    /// @brief Returns button for \c button_id.
    ///
    /// @param button_id Button ID of button to get from collection of monitored buttons.
    std::shared_ptr<PhoneButton> getButton(uint16_t const button_id);

    /// @brief Add phone UI to this lamp field.
    ///
    /// @param ui_name Phone name.
    /// @param ui Phone UI object.
    void registerUI(std::string const &ui_name, std::shared_ptr<ui::PhoneUI> const &ui);

    /// @brief Remove phone UI from this lamp field.
    ///
    /// @param ui_name Phone name.
    void unregisterUI(std::string const &ui_name);

    /// @brief Gets the phone UI for \c ui_name.
    ///
    /// @param ui_name Phone UI key.
    ///
    /// @return Pointer to phone UI object.
    std::shared_ptr<ui::PhoneUI> getPhoneUI(std::string const &ui_name);

protected:
    /// @brief Invalidates the lamp field object.
    ///
    /// @param button_id ID of button that invalidated the object.
    ///
    /// This function is invoked whenever a button state changes.
    void invalidate(uint16_t const button_id);

private:
    std::shared_ptr<ui::PhoneBridge> phone_bridge_;                           ///< Bridge to update hardware deskphones.
    std::unordered_map<uint16_t, std::shared_ptr<PhoneButton>> buttons_;      ///< Collection of observed buttons.
    std::shared_mutex buttons_mut_;                                           ///< Mutex on button collection.
    std::unordered_map<std::string, std::shared_ptr<ui::PhoneUI>> phone_uis_; ///< Phone UI's to render this object.
    std::shared_mutex phone_uis_mut_;                                         ///< Mutex on phone UI collection.
};

} // namespace button_state

#endif
