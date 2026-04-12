// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "monitor/yealink/NightButton.hpp"

#include "monitor/yealink/LampField.hpp"
#include <c++ami/action/Setvar.hpp>
#include <cassert>
#include <fmt/core.h>

using namespace monitor::yealink;

NightButton::NightButton(std::shared_ptr<lamp_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string night_exten, std::string context,
                         std::string device)
    : monitor::NightButton(std::move(phone_button), std::move(io_conn), std::move(night_exten), std::move(context),
                           std::move(device))
{
}

std::string NightButton::resetNightButtonState()
{
    auto const phone_button = getPhoneButton();
    assert(phone_button);
    auto const button_on = !phone_button->isOn();

    // Update device state on Asterisk server
    auto const io_conn = getAMIConnection();
    assert(io_conn);
    cpp_ami::action::Setvar action;
    action["Variable"] = fmt::format("DEVICE_STATE({})", getDevice());
    action["Value"] = button_on ? "INUSE" : "NOT_INUSE";
    io_conn->asyncInvoke(action);

    // Return XML for new button state
    auto const inverted_button = std::make_shared<lamp_state::PhoneButton>(
        phone_button->buttonID(), phone_button->color(), phone_button->flash(), button_on, button_on);
    return LampFieldState::toString(LampField::createPhoneStateXML({inverted_button}));
}
