// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "asterisk/ParkEventHandler.hpp"

#include <c++ami/action/ParkedCalls.hpp>
#include <cassert>
#include <syslog.h>

using namespace asterisk;

ParkEventHandler::ParkEventHandler(std::weak_ptr<button_state::PhoneButton> phone_button,
                                   std::shared_ptr<cpp_ami::Connection> io_conn, std::string parking_lot)
    : EventHandler(io_conn)
    , parking_lot_(std::move(parking_lot))
    , phone_button_(std::move(phone_button))
{
    assert(!parking_lot_.empty());
    assert(io_conn);
    ami_callback_id_ =
        io_conn->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    // Asterisk will return a list of ParkedCall events upon receiving an ParkedCalls action. Just have Asterisk send
    // the list so this objects event handler can take care of the event(s).
    cpp_ami::action::ParkedCalls const action;
    io_conn->asyncInvoke(action);
}

ParkEventHandler::~ParkEventHandler()
{
    auto const io_conn = getConnection();
    assert(io_conn);
    io_conn->removeCallback(ami_callback_id_);
}

std::shared_ptr<ParkEventHandler> ParkEventHandler::create(YAML::Node const &config,
                                                           std::weak_ptr<button_state::PhoneButton> const &phone_button,
                                                           std::shared_ptr<cpp_ami::Connection> const &conn)
{
    return std::make_shared<ParkEventHandler>(phone_button, conn, config["parking_lot"].as<std::string>());
}

EventHandler::EventType ParkEventHandler::getType() const
{
    return EventType::Park;
}

std::string ParkEventHandler::getParkingLot() const
{
    return parking_lot_;
}

void ParkEventHandler::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }

    static std::unordered_set<std::string> const park_events{"ParkedCall"};
    static std::unordered_set<std::string> const unpark_events{"ParkedCallGiveUp", "ParkedCallTimeOut", "UnParkedCall"};
    if (park_events.contains(event_type.value())) {
        if (event["Parkinglot"] == parking_lot_) {
            std::string const extension = event["ParkingSpace"];
            parked_extens_.emplace(extension);
            updateButtonState(parked_extens_.size() > 0);
        }
    }
    else if (unpark_events.contains(event_type.value())) {
        if (event["Parkinglot"] == parking_lot_) {
            std::string const extension = event["ParkingSpace"];
            parked_extens_.erase(extension);
            updateButtonState(parked_extens_.size() > 0);
        }
    }
}

void ParkEventHandler::updateButtonState(bool const parked_calls_present)
{
    syslog(LOG_DEBUG, "ParkButton::updateButtonState : Setting park button '%s'", parked_calls_present ? "on" : "off");

    auto const phone_button = phone_button_.lock();
    assert(phone_button);
    phone_button->setOn(parked_calls_present);
}
