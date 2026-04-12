// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "monitor/PhoneUIAdapter.hpp"
#include <cassert>
#include <fmt/core.h>
#include <shared_mutex>

using namespace monitor;

void PhoneUIAdapter::update(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons)
{
    // Create phone state XML from buttons
    bool critical{};
    auto xml_doc = createPhoneStateXML(buttons, critical);
    // Create new phone state object for caching
    auto const state = std::make_shared<PhoneUIState>(std::move(xml_doc), critical);
    setPhoneState(state);
}

std::shared_ptr<PhoneUIState> PhoneUIAdapter::getPhoneState()
{
    std::shared_lock const lock(cached_button_state_mut_);
    return cached_button_state_;
}

void PhoneUIAdapter::setPhoneState(std::shared_ptr<PhoneUIState> const &state)
{
    std::lock_guard const lock(cached_button_state_mut_);
    cached_button_state_ = state;
}

std::string PhoneUIAdapter::toString()
{
    auto const state = getPhoneState();
    return state->toString();
}

bool PhoneUIAdapter::isCritical()
{
    auto const state = getPhoneState();
    return state->isCritical();
}

void PhoneUIAdapter::initialize(cpp_ami::action::PJSIPNotify &action)
{
    action.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(toString()))});
}

char const *PhoneUIAdapter::toColorString(button_state::PhoneButton::Color const color)
{
    static constexpr char const *const BUTTON_COLOR_RED = "RED";
    static constexpr char const *const BUTTON_COLOR_GREEN = "GREEN";
    static constexpr char const *const BUTTON_COLOR_BLUE = "BLUE";

    char const *button_color;
    switch (color) {
    case button_state::PhoneButton::Color::Red:
        button_color = BUTTON_COLOR_RED;
        break;
    case button_state::PhoneButton::Color::Green:
        button_color = BUTTON_COLOR_GREEN;
        break;
    case button_state::PhoneButton::Color::Blue:
        button_color = BUTTON_COLOR_BLUE;
        break;
    default:
        assert(false);
    }
    return button_color;
}

char const *PhoneUIAdapter::toButtonStateString(button_state::PhoneButton const &button)
{
    static constexpr char const *const BUTTON_STATE_OFF = "off";
    static constexpr char const *const BUTTON_STATE_ON = "on";
    static constexpr char const *const BUTTON_STATE_FLASH = "slowflash";

    char const *button_state = BUTTON_STATE_OFF;
    if (button.isOn()) {
        button_state = button.flash() ? BUTTON_STATE_FLASH : BUTTON_STATE_ON;
    }
    return button_state;
}

pugi::xml_document
    PhoneUIAdapter::createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons)
{
    bool critical = false;
    return createPhoneStateXML(buttons, critical);
}

pugi::xml_document
    PhoneUIAdapter::createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons,
                                      bool &critical)
{
    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto execute_xml = xml_doc.append_child("YealinkIPPhoneExecute");
    execute_xml.append_attribute("refresh") = "1";

    assert(!buttons.empty());
    for (auto const &button : buttons) {
        critical |= button->isCritical();

        auto button_xml = execute_xml.append_child("ExecuteItem");
        button_xml.append_attribute("URI") = fmt::format("Led:LINE{}_{}={}", button->buttonID(),
                                                         toColorString(button->color()), toButtonStateString(*button));
    }
    execute_xml.append_attribute("Beep") = critical ? "yes" : "no";

    return xml_doc;
}
