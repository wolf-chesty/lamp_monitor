// Copyright (c) 2026 Christopher L Walkerlamp_state
// SPDX-License-Identifier: MIT

#include "monitor/NightButton.hpp"

#include <c++ami/action/ExtensionStateList.hpp>
#include <cassert>

using namespace monitor;

NightButton::NightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string night_exten, std::string context,
                         std::string device)
    : phone_button_(std::move(phone_button))
    , io_conn_(std::move(io_conn))
    , night_exten_(std::move(night_exten))
    , context_(std::move(context))
    , device_(std::move(device))
{
    assert(phone_button_);
    assert(io_conn_);
    ami_callback_id_ =
        io_conn_->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    // Asterisk will return a list of ExtensionStatus events upon receiving an ExtensionStateList action. Just have
    // Asterisk send the list so this objects event handler can take care of the event(s).
    cpp_ami::action::ExtensionStateList const action;
    io_conn_->asyncInvoke(action);
}

NightButton::~NightButton()
{
    assert(io_conn_);
    io_conn_->removeCallback(ami_callback_id_);
}

void NightButton::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    // Filter unmonitored messages
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }
    static std::unordered_set<std::string> const valid_events{"ExtensionStatus"};
    if (!valid_events.contains(event_type.value())) {
        return;
    }
    if (event["Context"] != context_ || event["Exten"] != night_exten_) {
        return;
    }

    // Process AMI event
    auto const &device_state = event["Status"];
    if (device_state == "0") {
        assert(phone_button_);
        phone_button_->setOn(false);
    }
    else if (device_state == "1") {
        assert(phone_button_);
        phone_button_->setOn(true);
    }
}

std::shared_ptr<cpp_ami::Connection> NightButton::getAMIConnection()
{
    return io_conn_;
}

std::string const &NightButton::getDevice()
{
    return device_;
}

std::shared_ptr<button_state::PhoneButton> NightButton::getPhoneButton()
{
    return phone_button_;
}
