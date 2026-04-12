// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_state/PhoneButton.hpp"

#include "lamp_state/LampField.hpp"

using namespace lamp_state;

PhoneButton::PhoneButton(uint16_t const button_id, Color const color, bool const flash, bool const is_critical,
                         bool const button_on)
    : PhoneButton(nullptr, button_id, color, flash, is_critical, button_on)
{
}

PhoneButton::PhoneButton(std::shared_ptr<LampField> lamp_field, uint16_t const button_id, Color const color,
                         bool const flash, bool const is_critical, bool const button_on)
    : lamp_field_(std::move(lamp_field))
    , color_(color)
    , flash_(flash)
    , button_id_(button_id)
    , button_on_(button_on)
    , is_critical_(is_critical)
{
}

uint16_t PhoneButton::buttonID() const
{
    return button_id_;
}

PhoneButton::Color PhoneButton::color() const
{
    return color_;
}

bool PhoneButton::flash() const
{
    return flash_;
}

bool PhoneButton::isOn() const
{
    return button_on_;
}

bool PhoneButton::setOn(bool const on)
{
    if (on == button_on_.exchange(on)) {
        return on;
    }
    updateLampField();
    return !button_on_;
}

bool PhoneButton::isCritical() const
{
    return is_critical_;
}

bool PhoneButton::setIsCritical(bool const update_required)
{
    if (update_required == is_critical_.exchange(update_required)) {
        return update_required;
    }
    updateLampField();
    return !is_critical_;
}

bool PhoneButton::setState(bool const is_on, bool const is_critical)
{
    auto const is_on_changed =is_on != button_on_.exchange(is_on);
    auto const is_critical_changed = is_critical != is_critical_.exchange(is_critical);
    if (is_on_changed || is_critical_changed) {
        updateLampField();
    }
    return is_on_changed || is_critical_changed;
}

void PhoneButton::updateLampField()
{
    if (lamp_field_) {
        lamp_field_->invalidate(button_id_);
    }
}
