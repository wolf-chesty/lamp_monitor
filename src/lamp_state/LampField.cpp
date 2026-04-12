// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_state/LampField.hpp"

#include "lamp_state/LampFieldObserver.hpp"
#include <cassert>
#include <execution>

using namespace lamp_state;

void LampField::registerObserver(LampFieldObserver *observer)
{
    assert(observer);
    std::lock_guard const lock(observers_mut_);
    observers_.emplace(observer);

    observer->invalidate(getButtons());
}

void LampField::unregisterObserver(LampFieldObserver *observer)
{
    assert(observer);
    std::lock_guard const lock(observers_mut_);
    observers_.erase(observer);
}

/// This function will create new phone buttons in the case where a phone button with \c button_id doesn't exist. In the
/// event that a button object exists with a button id of \c button_id, the existing button is returned.
std::shared_ptr<PhoneButton> LampField::getButton(uint16_t const button_id, PhoneButton::Color const color,
                                                  bool const flash, bool const critical, bool const on)
{
    std::lock_guard const lock(buttons_mut_);

    auto const it = buttons_.find(button_id);
    if (it == buttons_.end()) {
        auto button = std::make_shared<PhoneButton>(shared_from_this(), button_id, color, flash, critical, on);
        buttons_.emplace(button_id, button);
        return button;
    }
    return it->second;
}

void LampField::removeButton(uint16_t const button_id)
{
    std::lock_guard const lock(buttons_mut_);
    buttons_.erase(button_id);
}

std::vector<std::shared_ptr<PhoneButton>> LampField::getButtons()
{
    std::lock_guard const lock(buttons_mut_);
    std::vector<std::shared_ptr<PhoneButton>> buttons;
    std::ranges::transform(buttons_.begin(), buttons_.end(), std::back_inserter(buttons),
                           [](auto const &itr) -> std::shared_ptr<PhoneButton> { return itr.second; });
    return buttons;
}

void LampField::invalidate(uint16_t const button_id)
{
    std::lock_guard const lock(observers_mut_);
    std::for_each(std::execution::par, observers_.begin(), observers_.end(),
                  [buttons = getButtons()](auto const &observer) -> void { observer->invalidate(buttons); });
}
