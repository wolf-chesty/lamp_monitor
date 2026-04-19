// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "asterisk/RegisterEventHandler.hpp"

#include <cassert>

using namespace asterisk;

RegisterEventHandler::RegisterEventHandler(std::shared_ptr<DeskphoneCache> deskphone_cache,
                                           std::shared_ptr<cpp_ami::Connection> io_conn)
    : EventHandler(io_conn)
    , deskphone_cache_(std::move(deskphone_cache))
{
    assert(deskphone_cache_);
    assert(io_conn);
    ami_callback_id_ =
        io_conn->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });
}

RegisterEventHandler::~RegisterEventHandler()
{
    auto const io_conn = getConnection();
    assert(io_conn);
    io_conn->removeCallback(ami_callback_id_);
}

EventHandler::EventType RegisterEventHandler::getType() const
{
    return EventType::Register;
}

bool RegisterEventHandler::addButtonPlan(std::string const &name,
                                         std::shared_ptr<button_state::ButtonPlan> const &button_plan)
{
    std::lock_guard const lock(button_plans_mut_);
    auto const [_, success] = button_plans_.emplace(name, button_plan);
    return success;
}

std::shared_ptr<button_state::ButtonPlan> RegisterEventHandler::getButtonPlan(std::string const &name)
{
    std::shared_lock const lock(button_plans_mut_);
    auto const itr = button_plans_.find(name);
    return itr != button_plans_.end() ? itr->second : nullptr;
}

std::shared_ptr<ui::PhoneUI> RegisterEventHandler::getPhoneUI(std::string const &plan_name, std::string const &ui_name)
{
    auto const plan = getButtonPlan(plan_name);
    if (!plan) {
        return nullptr;
    }

    auto const ui = plan->getPhoneUI(ui_name);
    return ui;
}

/// This callback is invoked for every AMI event that is published by the Asterisk server. This callback will process
/// SuccessfulAuth events from the PJSIP service, whenever a deskphone has successfully registered with the Asterisk
/// server. Upon successful registration of a deskphone this callback will publish the current lamp states to the newly
/// registered deskphone.
void RegisterEventHandler::amiEventHandler(cpp_ami::util::KeyValDict const &event)
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

    // Our site currently only has one phone, the Yealink T88W's. Need away to determine which group the phone belong to
    // and the phones UI type.
    auto const phone_ui = getPhoneUI("default", "yealink_t88w");
    if (!phone_ui) {
        return;
    }

    // Grab deskphone adapter; the phone adapter knows how to render the screens for this particular endpoint.
    auto const &aor = event["AccountID"];

    // Some phones (like Android based Yealink deskphones) will wake the screen whenever they receive a PJSIP notify
    // message. This can cause wear on the backlight mechanism of the deskphone. Make sure to only publish the phone
    // state if the phone wasn't present for the previous lamp state change or the state requires the screen to be
    // turned on (i.e., in order to catch the users attention).
    if (deskphone_cache_->addEndpoint(aor, event["RemoteAddress"]) || phone_ui->isCritical()) {
        publishPhoneState(aor, phone_ui);
    }
}

void RegisterEventHandler::publishPhoneState(std::string const &aor, std::shared_ptr<ui::PhoneUI> const &phone_ui)
{
    cpp_ami::action::PJSIPNotify action;
    action["Endpoint"] = aor;
    assert(phone_ui);
    phone_ui->initialize(action);

    auto const io_conn = getConnection();
    assert(io_conn);
    io_conn->asyncInvoke(action);
}
