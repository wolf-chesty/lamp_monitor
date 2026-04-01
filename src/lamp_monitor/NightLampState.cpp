// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/NightLampState.hpp"

#include <fmt/core.h>

NightLampState::NightLampState(std::shared_ptr<cpp_ami::Connection> io_conn, uint8_t button_id, bool button_on)
    : LampMonitor(std::move(io_conn), button_id)
    , button_on_(button_on)
{
}

bool NightLampState::getButtonState()
{
    return button_on_;
}

bool NightLampState::setButtonState(bool button_on)
{
    return button_on_.exchange(button_on);
}

bool NightLampState::needsBeep() const
{
    return false;
}

void NightLampState::getButtonState(pugi::xml_node button_state_node) const
{
    button_state_node.append_attribute("URI") =
        fmt::format("Led:LINE{}_RED={}", getButtonId(), button_on_ ? "on" : "off");
}
