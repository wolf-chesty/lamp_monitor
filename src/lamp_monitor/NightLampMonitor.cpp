// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/NightLampMonitor.hpp"

#include <c++ami/action/ExtensionStateList.hpp>
#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/action/Setvar.hpp>
#include <cassert>
#include <fmt/core.h>
#include <future>

NightLampMonitor::NightLampMonitor(std::shared_ptr<cpp_ami::Connection> const &ami_conn, uint8_t button_id,
                                   std::string night_exten, std::string context, std::string device)
    : LampMonitor(ami_conn, button_id)
    , park_exten_(std::move(night_exten))
    , context_(std::move(context))
    , device_(std::move(device))
{
    assert(ami_conn);

    ami_callback_id_ =
        ami_conn->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    initLampState(ami_conn);
}

NightLampMonitor::~NightLampMonitor()
{
    auto const ami_conn = getAMIConnection();
    assert(ami_conn);
    ami_conn->removeCallback(ami_callback_id_);
}

void NightLampMonitor::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    static std::unordered_set<std::string> const valid_events{"ExtensionStatus"};
    if (auto const event_type = event.getValue("Event")) {
        if (valid_events.contains(event_type.value()) && event["Context"] == context_ &&
            event["Exten"] == park_exten_) {
            updateLampState(event["Status"]);
        }
    }
}

void NightLampMonitor::initLampState(std::shared_ptr<cpp_ami::Connection> const &ami_conn)
{
    assert(ami_conn);

    // Have Asterisk server send the current lamp state
    cpp_ami::action::ExtensionStateList const action;
    ami_conn->asyncInvoke(action);
}

void NightLampMonitor::updateLampState(std::string_view device_state)
{
    bool publish_button_state{false};
    if (device_state == "0") {
        publish_button_state = button_on_.exchange(false);
    }
    else if (device_state == "1") {
        publish_button_state = !button_on_.exchange(true);
    }

    if (publish_button_state) {
        sendButtonStateToPhones(button_on_);
    }
}

std::string NightLampMonitor::resetNightState()
{
    // Flip button on for XML generation
    button_on_ = !button_on_;

    // Update device state on Asterisk server
    auto const ami_conn = getAMIConnection();
    assert(ami_conn);
    cpp_ami::action::Setvar set_var;
    set_var["Variable"] = fmt::format("DEVICE_STATE({})", device_);
    set_var["Value"] = button_on_ ? "INUSE" : "NOT_INUSE";
    ami_conn->asyncInvoke(set_var);

    // Return button state XML
    return getButtonStateXML(true);
}

bool NightLampMonitor::needsBeep() const
{
    return false;
}

void NightLampMonitor::getButtonState(pugi::xml_node button_state_node) const
{
    button_state_node.append_attribute("URI") =
        fmt::format("Led:LINE{}_RED={}", getButtonId(), button_on_ ? "on" : "off");
}
