// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "xml/yealink/HttpNightButton.hpp"

#include "xml/yealink/PhoneUi.hpp"
#include <c++ami/action/Setvar.hpp>
#include <cassert>
#include <fmt/core.h>

using namespace xml::yealink;

HTTPNightButton::HTTPNightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                                 std::shared_ptr<cpp_ami::Connection> io_conn, std::string device)
    : button_(std::move(phone_button))
    , io_conn_(std::move(io_conn))
    , device_(std::move(device))
{
    assert(button_);
    assert(io_conn_);
}

std::string HTTPNightButton::pushButton()
{
    assert(button_);
    auto const button_on = !button_->isOn();

    // Update device state on Asterisk server
    assert(io_conn_);
    cpp_ami::action::Setvar action;
    action["Variable"] = fmt::format("DEVICE_STATE({})", device_);
    action["Value"] = button_on ? "INUSE" : "NOT_INUSE";
    io_conn_->asyncInvoke(action);

    // Return XML for new button state
    auto const inverted_button = std::make_shared<button_state::PhoneButton>(button_->buttonID(), button_->color(),
                                                                             button_->flash(), button_on, button_on);
    return PhoneUI::createYealinkXMLString({inverted_button});
}
