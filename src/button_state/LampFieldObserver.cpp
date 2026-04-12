// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "button_state/LampFieldObserver.hpp"

#include "button_state/LampField.hpp"

using namespace button_state;

std::vector<std::shared_ptr<PhoneButton>> LampFieldObserver::getButtons()
{
    std::shared_lock const lock(lamp_field_mut_);
    if (lamp_field_) {
        return lamp_field_->getButtons();
    }
    return std::vector<std::shared_ptr<PhoneButton>>{};
}

void LampFieldObserver::setLampField(std::shared_ptr<LampField> const &lamp_field)
{
    std::lock_guard const lock(lamp_field_mut_);
    lamp_field_ = lamp_field;
}

void LampFieldObserver::resetLampField()
{
    std::lock_guard const lock(lamp_field_mut_);
    lamp_field_.reset();
}
