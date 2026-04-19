// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "xml/yealink/HttpNightButton.hpp"

#include "xml/yealink/PhoneUi.hpp"
#include <c++ami/action/Setvar.hpp>
#include <c++ami/util/ScopeGuard.hpp>
#include <cassert>
#include <fmt/core.h>
#include <syslog.h>

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

std::string HTTPNightButton::httpPushButton()
{
    assert(button_);
    auto const button_on = !button_->isOn();

    // Update device state on Asterisk server
    cpp_ami::util::ScopeGuard const post_action([io_conn = io_conn_, device = device_, button_on]() -> void {
        assert(io_conn);
        cpp_ami::action::Setvar action;
        action["Variable"] = fmt::format("DEVICE_STATE({})", device);
        action["Value"] = button_on ? "INUSE" : "NOT_INUSE";
        syslog(LOG_DEBUG, "HTTPNightButton::pushButton() : Posting AMI action \"%s\"", action.toString().c_str());
        io_conn->asyncInvoke(action);
    });

    // Return XML for new button state
    syslog(LOG_DEBUG, "HTTPNightButton::pushButton() : Setting night button to \"%s\"", button_on ? "on" : "off");
    auto const inverted_button = button_->clone();
    inverted_button->setOn(button_on);
    return PhoneUI::createYealinkXMLString(inverted_button);
}

std::string HTTPNightButton::getContentType()
{
    static std::string content_type{"text/xml"};
    return content_type;
}