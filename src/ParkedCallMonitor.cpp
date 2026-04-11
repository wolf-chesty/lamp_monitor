// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ParkedCallMonitor.hpp"

#include <c++ami/action/ParkedCalls.hpp>
#include <cassert>
#include <fmt/core.h>

ParkedCallMonitor::ParkedCallMonitor(std::shared_ptr<cpp_ami::Connection> const &io_conn, uint8_t button_id)
    : LampMonitor(io_conn, button_id)
{
    assert(io_conn);
    parked_call_callback_id_ =
        io_conn->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    // Asterisk will return a list of ParkedCall events upon receiving an ParkedCalls action. Just have Asterisk send
    // the list so this objects event handler can take care of the event(s).
    cpp_ami::action::ParkedCalls parked_calls;
    io_conn->asyncInvoke(parked_calls);
}

ParkedCallMonitor::~ParkedCallMonitor()
{
    auto const ami_conn = getAMIConnection();
    assert(ami_conn);
    ami_conn->removeCallback(parked_call_callback_id_);
}

void ParkedCallMonitor::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }

    static std::unordered_set<std::string> const park_events{"ParkedCall"};
    static std::unordered_set<std::string> const unpark_events{"ParkedCallGiveUp", "ParkedCallTimeOut", "UnParkedCall"};
    bool park_event{false};
    if (park_events.contains(event_type.value())) {
        std::string const extension = event["ParkingSpace"];
        parked_extens_.emplace(extension);
        park_event = true;
    }
    else if (unpark_events.contains(event_type.value())) {
        std::string const extension = event["ParkingSpace"];
        parked_extens_.erase(extension);
        park_event = true;
    }

    if (park_event) {
        parked_call_count_ = parked_extens_.size();
        setButtonOn(parked_call_count_ > 0);
    }
}

bool ParkedCallMonitor::needsBeep() const
{
    return parked_call_count_ > 0;
}

void ParkedCallMonitor::getButtonState(pugi::xml_node button_state_node) const
{
    button_state_node.append_attribute("URI") =
        fmt::format("Led:LINE{}_RED={}", getButtonId(), parked_call_count_ > 0 ? "slowflash" : "off");
}

