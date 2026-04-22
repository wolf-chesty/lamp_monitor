// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "button_state/PhoneButton.hpp"

#include "button_state/ButtonPlan.hpp"
#include <syslog.h>

using namespace button_state;

PhoneButton::PhoneButton(std::weak_ptr<ButtonPlan> button_plan, uint16_t const button_id, Color const color,
                         FlashMode const flash_mode, bool const critical_when_on, bool const critical_when_off,
                         bool const button_on)
    : button_plan_(std::move(button_plan))
    , button_id_(button_id)
    , color_(color)
    , flash_mode_(flash_mode)
    , critical_when_on_(critical_when_on)
    , critical_when_off_(critical_when_off)
    , button_on_(button_on)
{
}

std::shared_ptr<PhoneButton> PhoneButton::create(YAML::Node const &config, std::weak_ptr<ButtonPlan> const &button_plan)
{
    return std::make_shared<PhoneButton>(button_plan, config["id"].as<uint16_t>(),
                                         toColor(config["color"].as<std::string>()),
                                         toFlashMode(config["flash"].as<std::string>()),
                                         config["critical_on"].as<bool>(), config["critical_off"].as<bool>(), false);
}

std::shared_ptr<PhoneButton> PhoneButton::clone()
{
    return std::make_shared<PhoneButton>(std::shared_ptr<ButtonPlan>{}, button_id_, color_, flash_mode_,
                                         critical_when_on_, critical_when_off_, button_on_);
}

PhoneButton::Color PhoneButton::toColor(std::string_view color_type)
{
    if (color_type == "red") {
        return Color::Red;
    }
    else if (color_type == "green") {
        return Color::Green;
    }
    else if (color_type == "blue") {
        return Color::Blue;
    }

    assert(false);
    syslog(LOG_WARNING, "Unknown button color type: %s", color_type.data());
    return Color::Red;
}

PhoneButton::FlashMode PhoneButton::toFlashMode(std::string_view flash_mode)
{
    if (flash_mode == "off") {
        return FlashMode::Off;
    }
    else if (flash_mode == "slow") {
        return FlashMode::Slow;
    }
    else if (flash_mode == "fast") {
        return FlashMode::Fast;
    }

    assert(false);
    syslog(LOG_WARNING, "Unknown flash mode: %s", flash_mode.data());
    return FlashMode::Off;
}

uint16_t PhoneButton::getButtonID() const
{
    return button_id_;
}

std::string PhoneButton::getLabel() const
{
    return label_;
}

PhoneButton::Color PhoneButton::getColor() const
{
    return color_;
}

PhoneButton::FlashMode PhoneButton::getFlashMode() const
{
    return flash_mode_;
}

bool PhoneButton::isOn() const
{
    return button_on_;
}

void PhoneButton::setOn(bool const on)
{
    if (on != button_on_.exchange(on)) {
        triggerLampFieldUpdate();
    }
}

bool PhoneButton::isCritical() const
{
    return button_on_ ? critical_when_on_ : critical_when_off_;
}

void PhoneButton::triggerLampFieldUpdate()
{
    if (auto const button_plan = button_plan_.lock()) {
        button_plan->invalidate(button_id_);
    }
}
