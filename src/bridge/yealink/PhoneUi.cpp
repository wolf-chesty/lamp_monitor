// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/yealink/PhoneUi.hpp"

#include "bridge/PhoneUiState.hpp"
#include <cassert>
#include <fmt/core.h>

using namespace bridge::yealink;

PhoneUI::PhoneUI(std::string name, std::shared_ptr<bridge::AoRProvider> adapter)
    : bridge::PhoneUI(std::move(name), std::move(adapter))
{
}

std::shared_ptr<bridge::PhoneUI> PhoneUI::create(YAML::Node const &config,
                                                 std::shared_ptr<bridge::AoRProvider> const &adapter)
{
    return std::make_shared<PhoneUI>(config["name"].as<std::string>(), adapter);
}

void PhoneUI::initialize(cpp_ami::action::PJSIPNotify &action)
{
    action.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(getStateString()))});
}

std::string PhoneUI::getHTTPState()
{
    return getStateString();
}

std::string PhoneUI::getContentType()
{
    static std::string const content_type("text/xml");
    return content_type;
}

char const *PhoneUI::toColorString(button_state::PhoneButton::Color const color)
{
    static constexpr char const *const BUTTON_COLOR_RED = "RED";
    static constexpr char const *const BUTTON_COLOR_GREEN = "GREEN";
    static constexpr char const *const BUTTON_COLOR_BLUE = "BLUE";

    switch (color) {
    case button_state::PhoneButton::Color::Red:
        return BUTTON_COLOR_RED;
    case button_state::PhoneButton::Color::Green:
        return BUTTON_COLOR_GREEN;
    case button_state::PhoneButton::Color::Blue:
        return BUTTON_COLOR_BLUE;
    default:
        assert(false);
    }
    return BUTTON_COLOR_RED;
}

char const *PhoneUI::toButtonStateString(button_state::PhoneButton const &button)
{
    static constexpr char const *const BUTTON_STATE_OFF = "off";
    static constexpr char const *const BUTTON_STATE_ON = "on";
    static constexpr char const *const BUTTON_STATE_FLASH_SLOW = "slowflash";
    static constexpr char const *const BUTTON_STATE_FLASH_FAST = "fastflash";

    if (button.isOn()) {
        switch (button.getFlashMode()) {
        case button_state::PhoneButton::FlashMode::Off:
            return BUTTON_STATE_ON;
        case button_state::PhoneButton::FlashMode::Fast:
            return BUTTON_STATE_FLASH_FAST;
        case button_state::PhoneButton::FlashMode::Slow:
            return BUTTON_STATE_FLASH_SLOW;
        default:
            assert(false);
        }
    }
    return BUTTON_STATE_OFF;
}

std::pair<pugi::xml_document, bool>
    PhoneUI::createPhoneStateXML(std::shared_ptr<button_state::PhoneButton> const &button, bool critical)
{
    return PhoneUI::createYealinkXML(button, critical);
}

std::pair<pugi::xml_document, bool>
    PhoneUI::createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical)
{
    return PhoneUI::createYealinkXML(buttons, critical);
}

std::string PhoneUI::createYealinkXMLString(std::shared_ptr<button_state::PhoneButton> const button)
{
    auto const [xml, _] = createYealinkXML(button, false);
    return bridge::PhoneUIState::toString(xml);
}

std::string PhoneUI::createYealinkXMLString(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons)
{
    auto const [xml, _] = createYealinkXML(buttons, false);
    return bridge::PhoneUIState::toString(xml);
}

std::pair<pugi::xml_document, bool> PhoneUI::createYealinkXML(std::shared_ptr<button_state::PhoneButton> const &button,
                                                              bool critical)
{
    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto execute_xml = xml_doc.append_child("YealinkIPPhoneExecute");
    execute_xml.append_attribute("refresh") = "1";

    assert(button);
    auto button_xml = execute_xml.append_child("ExecuteItem");
    button_xml.append_attribute("URI") = fmt::format("Led:LINE{}_{}={}", button->getButtonID(),
                                                     toColorString(button->getColor()), toButtonStateString(*button));
    critical |= button->isCritical();
    execute_xml.append_attribute("Beep") = critical ? "yes" : "no";

    return std::make_pair(std::move(xml_doc), critical);
}

std::pair<pugi::xml_document, bool>
    PhoneUI::createYealinkXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical)
{
    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto execute_xml = xml_doc.append_child("YealinkIPPhoneExecute");
    execute_xml.append_attribute("refresh") = "1";

    assert(!buttons.empty());
    for (auto const &button : buttons) {
        auto button_xml = execute_xml.append_child("ExecuteItem");
        button_xml.append_attribute("URI") = fmt::format(
            "Led:LINE{}_{}={}", button->getButtonID(), toColorString(button->getColor()), toButtonStateString(*button));
        critical |= button->isCritical();
    }
    execute_xml.append_attribute("Beep") = critical ? "yes" : "no";

    return std::make_pair(std::move(xml_doc), critical);
}
