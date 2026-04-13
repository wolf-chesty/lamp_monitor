// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ast_bridge/ParkButton.hpp"

#include <c++ami/action/ParkedCalls.hpp>
#include <cassert>

using namespace ast_bridge;

ParkButton::ParkButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                       std::shared_ptr<cpp_ami::Connection> io_conn)
    : phone_button_(std::move(phone_button))
    , io_conn_(std::move(io_conn))
{
    assert(phone_button_);
    assert(io_conn_);
    ami_callback_id_ =
        io_conn_->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    // Asterisk will return a list of ParkedCall events upon receiving an ParkedCalls action. Just have Asterisk send
    // the list so this objects event handler can take care of the event(s).
    cpp_ami::action::ParkedCalls const action;
    io_conn_->asyncInvoke(action);
}

ParkButton::~ParkButton()
{
    assert(io_conn_);
    io_conn_->removeCallback(ami_callback_id_);
}

void ParkButton::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }

    static std::unordered_set<std::string> const park_events{"ParkedCall"};
    static std::unordered_set<std::string> const unpark_events{"ParkedCallGiveUp", "ParkedCallTimeOut", "UnParkedCall"};
    if (park_events.contains(event_type.value())) {
        std::string const extension = event["ParkingSpace"];
        parked_extens_.emplace(extension);
        updateButtonState(parked_extens_.size() > 0);
    }
    else if (unpark_events.contains(event_type.value())) {
        std::string const extension = event["ParkingSpace"];
        parked_extens_.erase(extension);
        updateButtonState(parked_extens_.size() > 0);
    }
}

void ParkButton::updateButtonState(bool const parked_calls_present)
{
    phone_button_->setState(parked_calls_present, parked_calls_present);
}
