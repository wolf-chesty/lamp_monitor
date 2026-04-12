// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "button_state/PhoneButton.hpp"

#include "button_state/LampField.hpp"

using namespace button_state;

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

void PhoneButton::setOn(bool const on)
{
    if (on != button_on_.exchange(on)) {
        triggerLampFieldUpdate();
    }
}

bool PhoneButton::isCritical() const
{
    return is_critical_;
}

void PhoneButton::setIsCritical(bool const is_critical)
{
    if (is_critical != is_critical_.exchange(is_critical)) {
        triggerLampFieldUpdate();
    }
}

void PhoneButton::setState(bool const is_on, bool const is_critical)
{
    auto const is_on_changed = is_on != button_on_.exchange(is_on);
    auto const is_critical_changed = is_critical != is_critical_.exchange(is_critical);
    if (is_on_changed || is_critical_changed) {
        triggerLampFieldUpdate();
    }
}

void PhoneButton::triggerLampFieldUpdate()
{
    if (lamp_field_) {
        lamp_field_->invalidate(button_id_);
    }
}
