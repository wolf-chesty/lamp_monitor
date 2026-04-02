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
            auto const state = getCachedButtonState();
            if (handset_cache_.aorNeedsUpdate(aor) || state->forceUpdate()) {
                publishButtonState(aor, state->getXMLString());
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

void LampFieldMonitor::addLamps(std::list<std::shared_ptr<LampMonitor>> const &lamps)
{
    std::lock_guard const lock(lamps_mut_);
    for (auto const lamp : lamps) {
        lamps_.push_back(lamp);
        lamp->setLampFieldMonitor(shared_from_this());
    }

    invalidateButtonState();
}

std::shared_ptr<LampFieldState> LampFieldMonitor::getButtonState()
{
    std::lock_guard const lock(lamps_mut_);
    return getButtonState(lamps_, false);
}

std::shared_ptr<LampFieldState> LampFieldMonitor::getButtonState(std::vector<std::shared_ptr<LampMonitor>> const &lamps,
                                                                 bool beep)
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

    return std::make_shared<LampFieldState>(std::move(doc), beep);
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

        if (update_needed && button_state_thread_run_) {
            auto const state = cacheButtonState(getButtonState());
            publishButtonState(state->getXMLString());
        }
    }
}

void LampFieldMonitor::publishButtonState(std::string const &button_state_xml)
{
    cpp_ami::action::PJSIPNotify notify;
    notify.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});

    handset_cache_.clearValidAORs();

    // Get list of ContactStatusDetail events
    cpp_ami::action::PJSIPShowRegistrationInboundContactStatuses const list_action;
    auto const contact_list = io_conn_->invoke(list_action);
    // Notify all reachable contacts immediately
    std::unordered_set<std::string> unique_aors;
    contact_list->forEach([this, &notify, &unique_aors](cpp_ami::event::Event const &event) mutable -> bool {
        if (event["Status"] == "Reachable") {
            std::string const &aor = event["EndpointName"];
            auto const inserted_aor = unique_aors.insert(aor);
            if (inserted_aor.second) {
                notify["Endpoint"] = aor;
                io_conn_->asyncInvoke(notify);
            }
        }
        return false;
    });
}

void LampFieldMonitor::publishButtonState(std::string const &aor, std::string const &button_state_xml) const
{
    cpp_ami::action::PJSIPNotify action;
    action["Endpoint"] = aor;
    action.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});
    io_conn_->asyncInvoke(action);
}

std::shared_ptr<LampFieldState> LampFieldMonitor::cacheButtonState(std::shared_ptr<LampFieldState> state)
{
    // Cache the XML string
    std::lock_guard const lock(cached_state_mut_);
    cached_state_ = state;
    return cached_state_;
}

std::shared_ptr<LampFieldState> LampFieldMonitor::getCachedButtonState()
{
    std::lock_guard const lock(cached_state_mut_);
    return cached_state_;
}

void LampFieldMonitor::invalidateAOR(std::string const &aor)
{
    handset_cache_.invalidateAOR(aor);
}
