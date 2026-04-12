// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "LampFieldMonitor.hpp"

#include <c++ami/action/PjsipNotify.hpp>
#include <c++ami/action/PjsipShowRegistrationInboundContactStatuses.hpp>
#include <cassert>
#include <fmt/core.h>
#include <sstream>

LampFieldMonitor::LampFieldMonitor(std::unique_ptr<DeskphoneCache> handset_cache,
                                   std::shared_ptr<cpp_ami::Connection> io_conn)
    : io_conn_(std::move(io_conn))
    , handset_cache_(std::move(handset_cache))
{
    assert(handset_cache_);
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

/// This callback is invoked for every AMI event that is published by the Asterisk server. This callback will process
/// SuccessfulAuth events from the PJSIP service. This event type is published every time a phone successfully registers
/// with the Asterisk server.
///
/// Upon successful registration of a deskphone this callback will publish the current lamp states to the phone.
void LampFieldMonitor::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    // Filtered monitored events
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }
    static std::unordered_set<std::string> const valid_events{"SuccessfulAuth"};
    if (!valid_events.contains(event_type.value())) {
        return;
    }

    // Android based Yealink deskphones will wake the screen whenever they receive a PJSIP notify message. This
    // can cause wear on the backlight mechanism of the deskphone. Make sure to only publish the phone state if
    // the phone wasn't present for the lamp state change or the state requires the screen to be on (i.e., in
    // order to catch the users attention).
    if (event["Service"] == "PJSIP") {
        auto const &aor = event["AccountID"];
        auto const state = getCachedButtonState();
        if (handset_cache_->addEndpoint(aor, event["RemoteAddress"]) || state->isCritical()) {
            publishButtonState(aor, state->toString());
        }
    }
}

void LampFieldMonitor::addLamp(std::shared_ptr<LampMonitor> const &lamp)
{
    std::lock_guard const lock(lamps_mut_);
    lamps_.emplace_back(lamp);
    lamp->setLampFieldMonitor(shared_from_this());

    invalidateButtonState();
}

void LampFieldMonitor::addLamps(std::list<std::shared_ptr<LampMonitor>> const &lamps)
{
    std::lock_guard const lock(lamps_mut_);
    for (auto const lamp : lamps) {
        assert(lamp);
        lamps_.emplace_back(lamp);
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

void LampFieldMonitor::invalidateAOR(std::string const &aor, std::string const &ip)
{
    handset_cache_->deleteEndpoint(aor, ip);
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

    pthread_setname_np(button_state_thread_.native_handle(), "lamp_field");
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
            publishButtonState(state->toString());
        }
    }
}

void LampFieldMonitor::publishButtonState(std::string const &button_state_xml) const
{
    cpp_ami::action::PJSIPNotify notify;
    notify.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});

    handset_cache_->forEachAOR([this, notify = std::move(notify)](std::string_view aor) mutable -> void {
        notify["Endpoint"] = aor;
        io_conn_->asyncInvoke(notify);
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

std::shared_ptr<LampFieldState> LampFieldMonitor::cacheButtonState(std::shared_ptr<LampFieldState> const &state)
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
