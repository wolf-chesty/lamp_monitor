// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "monitor/Deskphone.hpp"

#include <cassert>

using namespace monitor;

Deskphone::Deskphone(std::shared_ptr<DeskphoneCache> deskphone_cache, std::shared_ptr<cpp_ami::Connection> io_conn)
    : deskphone_cache_(std::move(deskphone_cache))
    , io_conn_(std::move(io_conn))
{
    assert(deskphone_cache_);

    assert(io_conn_);
    ami_callback_id_ =
        io_conn_->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    startWorkThread();
}

Deskphone::~Deskphone()
{
    assert(io_conn_);
    io_conn_->removeCallback(ami_callback_id_);

    stopWorkThread();
}

/// This callback is invoked for every AMI event that is published by the Asterisk server. This callback will process
/// SuccessfulAuth events from the PJSIP service, whenever a deskphone has successfully registered with the Asterisk
/// server. Upon successful registration of a deskphone this callback will publish the current lamp states to the newly
/// registered deskphone.
void Deskphone::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    // Filter for monitored events
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }
    static std::unordered_set<std::string> const valid_events{"SuccessfulAuth"};
    if (!valid_events.contains(event_type.value())) {
        return;
    }
    if (event["Service"] != "PJSIP") {
        return;
    }

    // Grab deskphone adapter; the phone adapter knows how to render the screens for this particular endpoint.
    auto const &aor = event["AccountID"];
    auto phone_adapter = getPhoneAdapter();
    if (!phone_adapter) {
        return;
    }

    // Some phones (like Android based Yealink deskphones) will wake the screen whenever they receive a PJSIP notify
    // message. This can cause wear on the backlight mechanism of the deskphone. Make sure to only publish the phone
    // state if the phone wasn't present for the previous lamp state change or the state requires the screen to be
    // turned on (i.e., in order to catch the users attention).
    if (deskphone_cache_->addEndpoint(aor, event["RemoteAddress"]) || phone_adapter->isCritical()) {
        publishPhoneState(aor, phone_adapter);
    }
}

void Deskphone::registerPhoneUI(std::shared_ptr<PhoneUIAdapter> const &ui_adapter)
{
    std::lock_guard const lock(deskphone_adapter_mut_);
    if (deskphone_adapter_ != ui_adapter) {
        deskphone_adapter_ = ui_adapter;
        ui_adapter->update(getButtons());
    }
}

void Deskphone::unregisterPhoneUI(std::shared_ptr<PhoneUIAdapter> const &ui_adapter)
{
    std::lock_guard const lock(deskphone_adapter_mut_);
    if (deskphone_adapter_ == ui_adapter) {
        deskphone_adapter_.reset();
    }
}

std::shared_ptr<PhoneUIAdapter> Deskphone::getPhoneAdapter()
{
    std::shared_lock const lock(deskphone_adapter_mut_);
    return deskphone_adapter_;
}

void Deskphone::invalidate([[maybe_unused]] std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons)
{
    button_state_valid_ = false;
    work_thread_cv_.notify_one();
}

void Deskphone::startWorkThread()
{
    work_thread_run_ = true;
    work_thread_ = std::thread(&Deskphone::workThread, this);

    pthread_setname_np(work_thread_.native_handle(), "lamp_field");
}

void Deskphone::stopWorkThread()
{
    work_thread_run_ = false;
    work_thread_cv_.notify_one();

    assert(work_thread_.joinable());
    work_thread_.join();
}

void Deskphone::workThread()
{
    std::mutex button_state_mut;
    while (work_thread_run_) {
        std::unique_lock lock(button_state_mut);
        work_thread_cv_.wait(lock, [this]() -> bool { return !work_thread_run_ || !button_state_valid_; });
        lock.unlock();

        if (!button_state_valid_.exchange(true) && work_thread_run_) {
            // Grap deskphone adapter
            if (auto const phone_adapter = getPhoneAdapter()) {
                phone_adapter->update(getButtons());
                // Publish button state to deskphones
                publishPhoneState(phone_adapter);
            }
        }
    }
}

void Deskphone::publishPhoneState(std::string const &aor, std::shared_ptr<PhoneUIAdapter> const &phone_adapter)
{
    cpp_ami::action::PJSIPNotify action;
    action["Endpoint"] = aor;
    assert(phone_adapter);
    phone_adapter->initialize(action);

    assert(io_conn_);
    io_conn_->asyncInvoke(action);
}

void Deskphone::publishPhoneState(std::shared_ptr<PhoneUIAdapter> const &phone_adapter)
{
    cpp_ami::action::PJSIPNotify action;
    assert(phone_adapter);
    phone_adapter->initialize(action);

    assert(io_conn_);
    deskphone_cache_->forEachAOR([this, action](std::string_view aor) mutable -> void {
        action["Endpoint"] = aor;
        io_conn_->asyncInvoke(action);
    });
}
