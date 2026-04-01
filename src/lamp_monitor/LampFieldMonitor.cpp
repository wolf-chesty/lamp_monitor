// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/LampFieldMonitor.hpp"

#include <c++ami/action/PjsipNotify.hpp>
#include <cassert>
#include <fmt/core.h>
#include <iostream>
#include <sstream>

LampFieldMonitor::LampFieldMonitor(std::shared_ptr<cpp_ami::Connection> io_conn)
    : io_conn_(std::move(io_conn))
{
    assert(io_conn_);

    ami_callback_id_ =
        io_conn_->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });
}

LampFieldMonitor::~LampFieldMonitor()
{
    io_conn_->removeCallback(ami_callback_id_);
}

void LampFieldMonitor::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    static std::unordered_set<std::string> const valid_events{"SuccessfulAuth"};
    if (auto const event_type = event.getValue("Event")) {
        if (valid_events.contains(event_type.value()) && event["Service"] == "PJSIP") {
            auto const &aor = event["AccountID"];
            std::lock_guard const lock(desk_phones_mut_);
            if (!desk_phones_.contains(aor)) {
                publishButtonState(event["AccountID"]);
                desk_phones_.emplace(aor);
            }
        }
    }
}

void LampFieldMonitor::addLamp(std::shared_ptr<LampMonitor> const &lamp)
{
    std::lock_guard const lock(lamps_mut_);
    lamps_.push_back(lamp);

    lamp->setLampFieldMonitor(shared_from_this());
}

void LampFieldMonitor::publishButtonState(std::string const &aor)
{
    // Get escaped button state XML
    auto const button_state_xml = cpp_ami::util::KeyValDict::escape(getButtonStateXML());
    // Encode Variable value into an array string for PJSIPNotify AMI action
    cpp_ami::action::PJSIPNotify pjsip_notify;
    pjsip_notify["Endpoint"] = aor;
    pjsip_notify.setValues(
        "Variable", {"Event=Yealink-xml", "Content-Type=application/xml", fmt::format("Content={}", button_state_xml)});
    // Push lamp field state to phone
    io_conn_->asyncInvoke(pjsip_notify);
}

std::string LampFieldMonitor::getButtonStateXML()
{
    std::lock_guard const lock(lamps_mut_);
    return getButtonStateXML(lamps_);
}

std::string LampFieldMonitor::getButtonStateXML(std::vector<std::shared_ptr<LampMonitor>> const &lamps, bool beep)
{
    assert(!lamps.empty());

    pugi::xml_document doc;
    auto decl = doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto execute_xml = doc.append_child("YealinkIPPhoneExecute");
    execute_xml.append_attribute("refresh") = "1";

    for (auto const &lamp : lamps) {
        beep |= lamp->needsBeep();
        lamp->getButtonState(execute_xml.append_child("ExecuteItem"));
    }
    execute_xml.append_attribute("Beep") = beep ? "yes" : "no";

    std::ostringstream doc_str;
    doc.save(doc_str, "", pugi::format_raw);
    return doc_str.str();
}

void LampFieldMonitor::clearPhoneCache()
{
    std::lock_guard const lock(desk_phones_mut_);
    desk_phones_.clear();
}

void LampFieldMonitor::addPhone(std::string phone)
{
    std::lock_guard const lock(desk_phones_mut_);
    desk_phones_.emplace(std::move(phone));
}
