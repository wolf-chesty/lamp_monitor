// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_STATE_LAMP_FIELD_HPP
#define LAMP_STATE_LAMP_FIELD_HPP

#include "lamp_state/PhoneButton.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lamp_state {

class LampFieldObserver;

/// @class LampField
/// @namespace lamp_state
///
/// @brief Object that observes the state of \c PhoneButton objects.
///
/// This object holds multiple \c PhoneButton objects and will monitor them for state changes. Upon a phone button state
/// change this object will notify any \c LampFieldObserver objects that are observing this object of the button state
/// change.
class LampField : public std::enable_shared_from_this<LampField> {
    friend class LampFieldObserver;
    friend class PhoneButton;

public:
    LampField() = default;
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

protected:
    /// @brief Registers \c observer to this object.
    ///
    /// @param observer New observer of this object.
    void registerObserver(LampFieldObserver *observer);

    /// @brief Unregisters \c observer from this object.
    ///
    /// @param observer Observer to remove from this object.
    void unregisterObserver(LampFieldObserver *observer);

    /// @brief Invalidates the lamp field object.
    ///
    /// @param button_id ID of button that invalidated the object.
    ///
    /// This function is invoked whenever a button state changes.
    void invalidate(uint16_t const button_id);

private:
    std::unordered_map<uint16_t, std::shared_ptr<PhoneButton>> buttons_; ///< Collection of observed buttons.
    std::mutex buttons_mut_;                            ///< Mutex controlling access to button collection.
    std::unordered_set<LampFieldObserver *> observers_; ///< Collection of observers monitoring this object.
    std::mutex observers_mut_;                          ///< Mutex controlling access to observer objects.
};

} // namespace lamp_state

#endif
