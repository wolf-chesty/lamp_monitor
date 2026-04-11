// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "NightLampMonitor.hpp"

#include "LampFieldMonitor.hpp"
#include <c++ami/action/ExtensionStateList.hpp>
#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/action/Setvar.hpp>
#include <cassert>
#include <fmt/core.h>
#include <future>
#include <sstream>

NightLampMonitor::NightLampMonitor(std::shared_ptr<cpp_ami::Connection> const &io_conn, uint8_t button_id,
                                   std::string night_exten, std::string context, std::string device)
    : NightLampState(io_conn, button_id, false)
    , park_exten_(std::move(night_exten))
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

NightLampMonitor::~NightLampMonitor()
{
    auto const ami_conn = getAMIConnection();
    assert(ami_conn);
    ami_conn->removeCallback(ami_callback_id_);
}

void NightLampMonitor::amiEventHandler(cpp_ami::util::KeyValDict const &event)
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
    if (event["Context"] != context_ || event["Exten"] != park_exten_) {
        return;
    }

    // Process AMI event
    auto const &device_state = event["Status"];
    if (device_state == "0") {
        setButtonOn(false);
    }
    else if (device_state == "1") {
        setButtonOn(true);
    }
}

std::string NightLampMonitor::resetNightState()
{
    auto const button_on = !getButtonOn();

    // Update device state on Asterisk server
    auto const ami_conn = getAMIConnection();
    assert(ami_conn);
    cpp_ami::action::Setvar set_var;
    set_var["Variable"] = fmt::format("DEVICE_STATE({})", device_);
    set_var["Value"] = button_on ? "INUSE" : "NOT_INUSE";
    ami_conn->asyncInvoke(set_var);

    // Return button state XML
    auto const inverted_lamp = std::make_shared<NightLampState>(nullptr, getButtonId(), button_on);
    auto const state = LampFieldMonitor::getButtonState({inverted_lamp}, true);
    // Convert to XML string
    std::ostringstream doc_str;
    state->getXML().save(doc_str, "", pugi::format_raw);
    return doc_str.str();
}
