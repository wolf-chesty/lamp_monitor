// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/LampFieldMonitor.hpp"

#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/action/PjsipShowRegistrationInboundContactStatuses.hpp>
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

    startWorkThread();
}

LampFieldMonitor::~LampFieldMonitor()
{
    io_conn_->removeCallback(ami_callback_id_);

    stopWorkThread();
}

void LampFieldMonitor::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    static std::unordered_set<std::string> const valid_events{"SuccessfulAuth"};
    if (auto const event_type = event.getValue("Event")) {
        if (valid_events.contains(event_type.value()) && event["Service"] == "PJSIP") {
            auto const &aor = event["AccountID"];
            if (aorNeedsUpdate(aor)) {
                publishButtonState(aor, getCachedButtonStateXML());
            }
        }
    }
}

void LampFieldMonitor::addLamp(std::shared_ptr<LampMonitor> const &lamp)
{
    std::lock_guard const lock(lamps_mut_);
    lamps_.push_back(lamp);
    lamp->setLampFieldMonitor(shared_from_this());

    invalidateButtonState();
}

std::string LampFieldMonitor::getButtonStateXML()
{
    std::lock_guard const lock(lamps_mut_);
    return getButtonStateXML(lamps_, false);
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

void LampFieldMonitor::invalidateButtonState()
{
    button_state_valid_ = false;
    button_state_cv_.notify_one();
}

void LampFieldMonitor::startWorkThread()
{
    button_state_thread_run_ = true;
    button_state_thread_ = std::thread(&LampFieldMonitor::workThread, this);

    std::string_view thread_name("lamp_field");
    assert(thread_name.length() <= 16);
    pthread_setname_np(button_state_thread_.native_handle(), thread_name.data());
}

void LampFieldMonitor::stopWorkThread()
{
    button_state_thread_run_ = false;
    button_state_cv_.notify_one();

    assert(button_state_thread_.joinable());
    button_state_thread_.join();
}

void LampFieldMonitor::workThread()
{
    std::mutex button_state_mut_;
    while (button_state_thread_run_) {
        std::unique_lock lock(button_state_mut_);
        button_state_cv_.wait(lock, [this]() -> bool { return !button_state_thread_run_ || !button_state_valid_; });
        auto const update_needed = !button_state_valid_.exchange(true);
        lock.unlock();

        if (button_state_thread_run_ && update_needed) {
            publishButtonState(setCachedButtonStateXML(getButtonStateXML()));
        }
    }
}

void LampFieldMonitor::publishButtonState(std::string const &button_state_xml)
{
    cpp_ami::action::PJSIPNotify notify;
    notify.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});

    clearValidAORs();

    // Get list of ContactStatusDetail events
    cpp_ami::action::PJSIPShowRegistrationInboundContactStatuses const list_action;
    auto const contact_list = io_conn_->invoke(list_action);
    // Notify all reachable contacts immediately
    std::unordered_set<std::string> unique_aors;
    contact_list->forEach([this, &notify, &unique_aors](cpp_ami::event::Event const &event) mutable -> bool {
        std::string const &aor = event["EndpointName"];
        auto const inserted_aor = unique_aors.insert(aor);
        if (event["Status"] == "Reachable") {
            if (inserted_aor.second) {
                notify["Endpoint"] = aor;
                io_conn_->asyncInvoke(notify);
            }
        }
        return false;
    });
}

void LampFieldMonitor::publishButtonState(std::string const &aor, std::string const &button_state_xml)
{
    cpp_ami::action::PJSIPNotify action;
    action["Endpoint"] = aor;
    action.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});
    io_conn_->asyncInvoke(action);
}

std::string const &LampFieldMonitor::setCachedButtonStateXML(std::string button_state_xml)
{
    std::lock_guard const lock(button_state_xml_mut_);
    button_state_xml_ = std::move(button_state_xml);
    return button_state_xml_;
}

std::string const &LampFieldMonitor::getCachedButtonStateXML()
{
    std::lock_guard const lock(button_state_xml_mut_);
    return button_state_xml_;
}

void LampFieldMonitor::clearValidAORs()
{
    std::lock_guard const lock(valid_aors_mut_);
    valid_aors_.clear();
}

bool LampFieldMonitor::aorNeedsUpdate(std::string const &aor)
{
    std::lock_guard const lock(valid_aors_mut_);
    auto inserted_aor = valid_aors_.insert(aor);
    return inserted_aor.second;
}
