// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_state/PhoneButton.hpp"

using namespace lamp_state;

PhoneButton::PhoneButton(std::shared_ptr<LampField> lamp_field, uint16_t button_id, Color color, bool flash)
    : lamp_field_(std::move(lamp_field))
    , color_(color)
    , flash_(flash)
    , button_id_(button_id)
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
    if (on != button_on_.exchange(on)) {
        if (lamp_field_) {
            lamp_field_->invalidate(button_id_);
        }
    }
    return button_on_;
}
