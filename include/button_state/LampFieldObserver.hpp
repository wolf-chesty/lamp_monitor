// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BUTTON_STATE_LAMP_FIELD_OBSERVER_HPP
#define BUTTON_STATE_LAMP_FIELD_OBSERVER_HPP

#include "button_state/PhoneButton.hpp"
#include <memory>
#include <shared_mutex>
#include <vector>

namespace button_state {

class LampField;

/// @class LampFieldObserver
/// @namespace button_state
///
/// @brief Objects of this type observe a \c LampField and will be notified of any button changes.
class LampFieldObserver {
    friend class LampField;

public:
    LampFieldObserver() = default;
    virtual ~LampFieldObserver() = default;

    /// @brief Function that is invoked whenever a button in the lamp field has its state changed.
    ///
    /// @param buttons Collection of buttons in the lamp field.
    virtual void invalidate(std::vector<std::shared_ptr<PhoneButton>> const &buttons) = 0;

protected:
    /// @brief Returns list of buttons in the lamp field.
    ///
    /// @return Collection of \c PhoneButton objects belonging to the lamp field.
    std::vector<std::shared_ptr<PhoneButton>> getButtons();

private:
    /// @brief Attaches this object to the \c lamp_field object.
    ///
    /// @param lamp_field Lamp field to attach this object to.
    void setLampField(std::shared_ptr<LampField> const &lamp_field);

    /// @brief Detaches this object from its observed lamp field.
    void resetLampField();

    std::shared_ptr<LampField> lamp_field_; ///< Pointer to lamp field object being monitored.
    std::shared_mutex lamp_field_mut_;      ///< Mutex on lamp field pointer.
};

} // namespace button_state

#endif
