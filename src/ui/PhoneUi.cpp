// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ui/PhoneUi.hpp"

#include <fmt/core.h>
#include <shared_mutex>
#include <syslog.h>

using namespace ui;

PhoneUI::PhoneUI()
{
    syslog(LOG_DEBUG, "PhoneUI::PhoneUI()");
}

PhoneUI::~PhoneUI()
{
    syslog(LOG_DEBUG, "PhoneUI::~PhoneUI()");
}

void PhoneUI::update(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons)
{
    syslog(LOG_DEBUG, "PhoneUI::update() : Caching phone screen state");

    // Create phone state XML from buttons
    auto [xml_doc, critical] = createPhoneStateXML(buttons, false);
    // Create new phone state object for caching
    setPhoneState(std::make_shared<PhoneUIState>(std::move(xml_doc), critical));
}

std::shared_ptr<PhoneUIState> PhoneUI::getPhoneState()
{
    std::shared_lock const lock(cached_button_state_mut_);
    return cached_button_state_;
}

void PhoneUI::setPhoneState(std::shared_ptr<PhoneUIState> const &state)
{
    std::lock_guard const lock(cached_button_state_mut_);
    cached_button_state_ = state;
}

std::string PhoneUI::toString()
{
    auto const state = getPhoneState();
    return state->toString();
}

bool PhoneUI::isCritical()
{
    auto const state = getPhoneState();
    return state->isCritical();
}
