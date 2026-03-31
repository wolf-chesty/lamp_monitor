// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/LampMonitor.hpp"

#include "lamp_monitor/LampFieldMonitor.hpp"
#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/action/PjsipShowRegistrationInboundContactStatuses.hpp>
#include <cassert>
#include <fmt/core.h>

LampMonitor::LampMonitor(std::shared_ptr<cpp_ami::Connection> ami_conn, uint8_t button_id)
    : ami_conn_(std::move(ami_conn))
    , button_id_(button_id)
{
    assert(button_id_ > 0);
}

std::shared_ptr<cpp_ami::Connection> LampMonitor::getAMIConnection() const
{
    return ami_conn_;
}

uint8_t LampMonitor::getButtonId() const
{
    return button_id_;
}

std::string LampMonitor::getButtonStateXML(bool const beep)
{
    return LampFieldMonitor::getButtonStateXML({shared_from_this()}, beep);
}

void LampMonitor::sendButtonStateToPhones(bool const beep)
{
    auto const button_state_xml = cpp_ami::util::KeyValDict::escape(getButtonStateXML(beep));
    cpp_ami::action::PJSIPNotify pjsip_notify;
    pjsip_notify.setValues(
        "Variable", {"Event=Yealink-xml", "Content-Type=application/xml", fmt::format("Content={}", button_state_xml)});

    std::unordered_set<std::string> unique_aors;

    // Get a list of ContactStatusDetail events
    cpp_ami::action::PJSIPShowRegistrationInboundContactStatuses const action;
    auto const result = ami_conn_->invoke(action);
    // Invoke command on all contacts
    result->forEach([this, &unique_aors, &pjsip_notify](cpp_ami::event::Event const &event) mutable -> bool {
        auto const &aor = event["EndpointName"];
        if (!unique_aors.contains(aor) && event["Status"] == "Reachable") {
            pjsip_notify["Endpoint"] = aor;
            ami_conn_->asyncInvoke(pjsip_notify);
            unique_aors.insert(aor);
        }
        return false;
    });
}
