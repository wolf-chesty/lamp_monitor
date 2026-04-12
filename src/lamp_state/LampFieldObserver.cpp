// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_state/LampFieldObserver.hpp"

using namespace lamp_state;

LampFieldObserver::LampFieldObserver(std::shared_ptr<LampField> lamp_field)
    : lamp_field_(std::move(lamp_field))
{
    if (lamp_field_) {
        lamp_field_->registerObserver(this);
    }
}

LampFieldObserver::~LampFieldObserver()
{
    if (lamp_field_) {
        lamp_field_->unregisterObserver(this);
    }
}

std::vector<std::shared_ptr<PhoneButton>> LampFieldObserver::getButtons()
{
    if (lamp_field_) {
        return lamp_field_->getButtons();
    }
    return std::vector<std::shared_ptr<PhoneButton>>{};
}
