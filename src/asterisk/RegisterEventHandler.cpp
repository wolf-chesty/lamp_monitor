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

void RegisterEventHandler::registerButtonPlan(std::shared_ptr<button_state::ButtonPlan> const &button_plan)
{
    std::lock_guard const lock(button_plans_mut_);
    button_plans_.emplace_back(button_plan);
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
    static std::unordered_set<std::string> const valid_services{"PJSIP"};
    if (!valid_services.contains(event["Service"])) {
        return;
    }

    auto const &aor = event["AccountID"];
    publishPhoneState(deskphone_cache_->addEndpoint(aor, event["RemoteAddress"]), aor);
}

void RegisterEventHandler::publishPhoneState(bool const new_aor, std::string const &aor)
{
    std::lock_guard const lock(button_plans_mut_);
    auto const io_conn = getConnection();
    for (auto const &button_plan : button_plans_) {
        for (auto const &phone_ui : button_plan->getPhoneUIs(aor)) {
            // Some phones (like Android based Yealink deskphones) will wake the screen whenever they receive a PJSIP
            // notify message. This can cause wear on the backlight mechanism of the deskphone. Make sure to only
            // publish the phone state if the phone wasn't present for the previous lamp state change or the screen is
            // required to be shown.
            if (!new_aor && !phone_ui->isCritical()) {
                continue;
            }

            // Construct PJSIP notify action
            cpp_ami::action::PJSIPNotify action;
            action["Endpoint"] = aor;
            assert(phone_ui);
            phone_ui->initialize(action);
            // Send PJSIP notify action to Asterisk server
            io_conn->asyncInvoke(action);
        }
    }
}
