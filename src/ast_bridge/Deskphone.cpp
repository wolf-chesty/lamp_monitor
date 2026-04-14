// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ast_bridge/Deskphone.hpp"

#include <cassert>

using namespace ast_bridge;

Deskphone::Deskphone(std::shared_ptr<DeskphoneCache> deskphone_cache,
                     std::shared_ptr<button_state::LampField> lamp_field, std::shared_ptr<cpp_ami::Connection> io_conn)
    : deskphone_cache_(std::move(deskphone_cache))
    , lamp_field_(std::move(lamp_field))
    , io_conn_(std::move(io_conn))
{
    assert(deskphone_cache_);

    assert(io_conn_);
    ami_callback_id_ =
        io_conn_->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });
}

Deskphone::~Deskphone()
{
    assert(io_conn_);
    io_conn_->removeCallback(ami_callback_id_);
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
    // Our site currently only has one phone, the Yealink T88W's. Perhaps we can determine the UI type from registration
    // details?
    auto const phone_ui = lamp_field_->getPhoneUI("t88w");
    if (!phone_ui) {
        return;
    }

    // Some phones (like Android based Yealink deskphones) will wake the screen whenever they receive a PJSIP notify
    // message. This can cause wear on the backlight mechanism of the deskphone. Make sure to only publish the phone
    // state if the phone wasn't present for the previous lamp state change or the state requires the screen to be
    // turned on (i.e., in order to catch the users attention).
    if (deskphone_cache_->addEndpoint(aor, event["RemoteAddress"]) || phone_ui->isCritical()) {
        publishPhoneState(aor, phone_ui);
    }
}

void Deskphone::publishPhoneState(std::string const &aor, std::shared_ptr<ui::PhoneUI> const &phone_ui)
{
    cpp_ami::action::PJSIPNotify action;
    action["Endpoint"] = aor;
    assert(phone_ui);
    phone_ui->initialize(action);

    assert(io_conn_);
    io_conn_->asyncInvoke(action);
}
