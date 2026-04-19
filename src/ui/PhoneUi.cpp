// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ui/PhoneUi.hpp"

#include "xml/yealink/PhoneUi.hpp"
#include <cassert>
#include <fmt/core.h>
#include <shared_mutex>
#include <syslog.h>

using namespace ui;

PhoneUI::PhoneUI(std::string name)
    : name_(std::move(name))
{
    syslog(LOG_DEBUG, "PhoneUI::PhoneUI()");
}

PhoneUI::~PhoneUI()
{
    syslog(LOG_DEBUG, "PhoneUI::~PhoneUI()");
}

std::pair<std::string, std::shared_ptr<PhoneUI>> PhoneUI::create(YAML::Node const &config)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "yealink") {
        return xml::yealink::PhoneUI::create(config);
    }
    assert(false);
    return std::make_pair("", nullptr);
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

std::string PhoneUI::getStateString()
{
    auto const state = getPhoneState();
    return state->toString();
}

bool PhoneUI::isCritical()
{
    auto const state = getPhoneState();
    return state->isCritical();
}

std::string PhoneUI::getName()
{
    return name_;
}
