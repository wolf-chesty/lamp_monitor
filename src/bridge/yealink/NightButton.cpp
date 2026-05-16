// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/yealink/NightButton.hpp"

#include "bridge/yealink/PhoneUi.hpp"
#include <syslog.h>

using namespace bridge::yealink;

NightButton::NightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                                 std::shared_ptr<cpp_ami::Connection> io_conn, std::string device)
    : bridge::NightButton(std::move(phone_button), std::move(io_conn), std::move(device))
{
}

std::string NightButton::getContentType()
{
    static std::string content_type{"text/xml"};
    return content_type;
}

std::string NightButton::pushButton(std::shared_ptr<button_state::PhoneButton> const &phone_button,
                                            bool button_on)
{
    // Return XML for new button state
    syslog(LOG_DEBUG, "HTTPNightButton::pushButton() : Setting night button to \"%s\"", button_on ? "on" : "off");
    phone_button->setOn(button_on);
    return PhoneUI::createYealinkXMLString(phone_button);
}
