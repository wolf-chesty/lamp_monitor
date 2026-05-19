// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "asterisk/ParkEventHandler.hpp"

#include <c++ami/action/ExtensionStateList.hpp>
#include <c++ami/action/GetConfigJson.hpp>
#include <c++ami/action/ParkedCalls.hpp>
#include <cassert>
#include <fmt/core.h>
#include <syslog.h>
#include <yaml-cpp/yaml.h>

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

    parking_hints_ = getParkingHints(io_conn, parking_lot_);

    // Asterisk will return a list of ExtensionStatus events upon receiving an ExtensionStateList action. Just have
    // Asterisk send the list to this objects event handler can take care of the event(s).
    cpp_ami::action::ExtensionStateList const action;
    io_conn->asyncInvoke(action);

    // Asterisk will return a list of ParkedCall events upon receiving an ParkedCalls action. Just have Asterisk send
    // the list so this objects event handler can take care of the event(s).
    // cpp_ami::action::ParkedCalls const action;
    // io_conn->asyncInvoke(action);
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
    /*
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
    */

    // Filter unmonitored messages
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }
    static std::unordered_set<std::string> const valid_events{"ExtensionStatus"};
    if (!valid_events.contains(event_type.value())) {
        return;
    }
    if (!parking_hints_.contains(event["Hint"])) {
        return;
    }

    // Process AMI event
    auto const &exten = event["Exten"];
    auto const &device_state = event["Status"];
    if (device_state == "0") {
        parked_extens_.erase(exten);
    }
    else if (device_state == "1") {
        parked_extens_.emplace(exten);
    }
    updateButtonState(parked_extens_.size() > 0);
}

void ParkEventHandler::updateButtonState(bool const parked_calls_present)
{
    syslog(LOG_DEBUG, "ParkButton::updateButtonState : Setting park button '%s'", parked_calls_present ? "on" : "off");

    auto const phone_button = phone_button_.lock();
    assert(phone_button);
    phone_button->setOn(parked_calls_present);
}

std::unordered_set<std::string> ParkEventHandler::getParkingHints(std::shared_ptr<cpp_ami::Connection> const &conn,
                                                                  std::string const &parking_lot)
{
    cpp_ami::action::GetConfigJSON action;
    action["Filename"] = "res_parking.conf";
    auto const response = conn->invoke(action);
    if (!response->isSuccess()) {
        syslog(LOG_WARNING, "Unable to read res_parking.conf");
        return std::unordered_set<std::string>();
    }

    std::unordered_set<std::string> parking_hints;
    response->forEach([parking_lot, &parking_hints](cpp_ami::event::Event const &event) mutable -> bool {
        auto const cfg_yaml = YAML::Load(event["JSON"]);
        auto const parking_yaml = cfg_yaml[parking_lot];
        if (!parking_yaml) {
            syslog(LOG_WARNING, "Missing parking lot %s from config file", parking_lot.c_str());
            return true;
        }

        auto const context = parking_yaml["context"].as<std::string>();
        auto const park_pos = parking_yaml["parkpos"].as<std::string>();
        auto const start_pos = std::stoi(park_pos);
        auto end_pos = start_pos;
        if (auto const pos = park_pos.find('-'); pos != std::string::npos) {
            end_pos = std::stoi(park_pos.substr(pos + 1));
        }
        syslog(LOG_DEBUG, "Parking lot %s using parking extensions: %d-%d", parking_lot.c_str(), start_pos, end_pos);

        for (auto pos = start_pos; pos <= end_pos; ++pos) {
            auto const hint = fmt::format("park:{}@{}", pos, context);
            syslog(LOG_DEBUG, "Adding hint %s to %s", hint.c_str(), parking_lot.c_str());
            parking_hints.emplace(hint);
        }

        return true;
    });
    return parking_hints;
}
