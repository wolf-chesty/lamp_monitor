// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/PhoneUi.hpp"

#include "bridge/yealink/PhoneUi.hpp"
#include <cassert>
#include <shared_mutex>
#include <syslog.h>

using namespace bridge;

PhoneUI::PhoneUI(std::string name, std::unique_ptr<bridge::AoRProvider> provider)
    : name_(std::move(name))
    , provider_(std::move(provider))
{
    assert(!name_.empty());
    assert(provider_ != nullptr);

    syslog(LOG_DEBUG, "PhoneUI::PhoneUI()");

    setAoRs(provider_->getCompatibleAoRs());
}

PhoneUI::~PhoneUI()
{
    syslog(LOG_DEBUG, "PhoneUI::~PhoneUI()");
}

std::shared_ptr<PhoneUI> PhoneUI::create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &io_conn)
{
    auto const name = config["name"].as<std::string>();
    auto provider = bridge::AoRProvider::create(config["provider"], io_conn);
    if (auto const &type = config["type"].as<std::string>(); type == "yealink") {
        return std::make_shared<bridge::yealink::PhoneUI>(name, std::move(provider));
    }

    assert(false);
    return nullptr;
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

bool PhoneUI::hasAoR(std::string const &aor)
{
    std::shared_lock const lock(compatible_aors_mut_);
    return compatible_aors_.contains(aor);
}

std::vector<std::string> PhoneUI::getAoRs()
{
    std::shared_lock const lock(compatible_aors_mut_);
    std::vector<std::string> aors(compatible_aors_.begin(), compatible_aors_.end());
    return aors;
}

void PhoneUI::setAoRs(std::unordered_set<std::string> aors)
{
    std::lock_guard const lock(compatible_aors_mut_);
    compatible_aors_ = std::move(aors);
}
