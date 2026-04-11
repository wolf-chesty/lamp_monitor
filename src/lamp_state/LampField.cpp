// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_state/LampField.hpp"

using namespace lamp_state;

std::shared_ptr<PhoneButton> LampField::getButton(uint16_t button_id, PhoneButton::Color color, bool flash)
{
    std::lock_guard const lock(buttons_mut_);

    if (auto const it = buttons_.find(button_id); it != buttons_.end()) {
        return it->second;
    }

    auto button = std::make_shared<PhoneButton>(shared_from_this(), button_id, color, flash);
    buttons_.emplace(button_id, button);
    return button;
}

std::vector<std::shared_ptr<PhoneButton>> LampField::getButtons()
{
    std::lock_guard const lock(buttons_mut_);
    std::vector<std::shared_ptr<PhoneButton>> buttons;
    std::transform(buttons_.begin(), buttons_.end(), std::back_inserter(buttons),
                   [](auto const &itr) -> std::shared_ptr<PhoneButton> { return itr.second; });
    return buttons;
}

void LampField::invalidate(uint16_t const button_id)
{
}
