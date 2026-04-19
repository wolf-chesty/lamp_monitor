// Copyright (c) 2026 Christopher L Walkerlamp_state
// SPDX-License-Identifier: MIT

#include "asterisk/NightEventHandler.hpp"

#include <c++ami/action/ExtensionStateList.hpp>
#include <cassert>
#include <syslog.h>

using namespace asterisk;

NightEventHandler::NightEventHandler(std::weak_ptr<button_state::PhoneButton> phone_button,
                                     std::shared_ptr<cpp_ami::Connection> io_conn, std::string night_exten,
                                     std::string context, std::string device)
    : EventHandler(io_conn)
    , phone_button_(std::move(phone_button))
    , night_exten_(std::move(night_exten))
    , context_(std::move(context))
    , device_(std::move(device))
{
    assert(io_conn);
    ami_callback_id_ =
        io_conn->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    // Asterisk will return a list of ExtensionStatus events upon receiving an ExtensionStateList action. Just have
    // Asterisk send the list so this objects event handler can take care of the event(s).
    cpp_ami::action::ExtensionStateList const action;
    io_conn->asyncInvoke(action);
}

NightEventHandler::~NightEventHandler()
{
    auto const io_conn = getConnection();
    assert(io_conn);
    io_conn->removeCallback(ami_callback_id_);
}

std::shared_ptr<NightEventHandler>
    NightEventHandler::create(YAML::Node const &config, std::weak_ptr<button_state::PhoneButton> const &phone_button,
                              std::shared_ptr<cpp_ami::Connection> const &conn)
{
    return std::make_shared<NightEventHandler>(phone_button, conn, config["exten"].as<std::string>(),
                                               config["context"].as<std::string>(), config["device"].as<std::string>());
}

EventHandler::EventType NightEventHandler::getType() const
{
    return EventType::Night;
}

std::string NightEventHandler::getDevice()
{
    return device_;
}

void NightEventHandler::amiEventHandler(cpp_ami::util::KeyValDict const &event)
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
        auto const phone_button = phone_button_.lock();
        assert(phone_button);
        syslog(
            LOG_DEBUG,
            "NightButton::amiEventHandler() : Setting night button 'off'; exten=\"%s\", context=\"%s\", device=\"%s\"",
            night_exten_.c_str(), context_.c_str(), device_.c_str());
        phone_button->setOn(false);
    }
    else if (device_state == "1") {
        auto const phone_button = phone_button_.lock();
        assert(phone_button);
        syslog(
            LOG_DEBUG,
            "NightButton::amiEventHandler() : Setting night button 'on'; exten=\"%s\", context=\"%s\", device=\"%s\"",
            night_exten_.c_str(), context_.c_str(), device_.c_str());
        phone_button->setOn(true);
    }
}
