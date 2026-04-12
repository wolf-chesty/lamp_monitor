// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_STATE_LAMP_FIELD_OBSERVER_HPP
#define LAMP_STATE_LAMP_FIELD_OBSERVER_HPP

#include "lamp_state/LampField.hpp"
#include "lamp_state/PhoneButton.hpp"
#include <memory>
#include <vector>

namespace lamp_state {

/// @class LampFieldObserver
/// @namespace lamp_state
///
/// @brief Objects of this type will be notified of button state changes for buttons that belong to the lamp field.
class LampFieldObserver {
public:
    explicit LampFieldObserver(std::shared_ptr<LampField> lamp_field);
    virtual ~LampFieldObserver();

    /// @brief Function that is invoked whenever a button in the lamp field has had its state changed.
    ///
    /// @param buttons Collection of buttons in the lamp field.
    virtual void invalidate(std::vector<std::shared_ptr<PhoneButton>> const &buttons) = 0;

protected:
    /// @brief Returns list of buttons in the lamp field.
    ///
    /// @return Collection of \c PhoneButton objects belonging to the lamp field.
    std::vector<std::shared_ptr<PhoneButton>> getButtons();

private:
    std::shared_ptr<LampField> lamp_field_; ///< Pointer to lamp field object being monitored.
};

} // namespace lamp_state

#endif
