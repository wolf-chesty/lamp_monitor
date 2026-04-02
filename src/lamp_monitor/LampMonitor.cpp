// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/LampMonitor.hpp"

#include "lamp_monitor/LampFieldMonitor.hpp"
#include <cassert>
#include <fmt/core.h>

LampMonitor::LampMonitor(std::shared_ptr<cpp_ami::Connection> io_conn, uint8_t button_id, bool button_on)
    : io_conn_(std::move(io_conn))
    , button_id_(button_id)
    , button_on_(button_on)
{
}

std::shared_ptr<cpp_ami::Connection> LampMonitor::getAMIConnection() const
{
    return io_conn_;
}

uint8_t LampMonitor::getButtonId() const
{
    return button_id_;
}

bool LampMonitor::getButtonOn() const
{
    return button_on_.load();
}

bool LampMonitor::setButtonOn(bool button_on)
{
    auto const prev_state = button_on_.exchange(button_on);
    if (prev_state != button_on) {
        invalidateButtonState();
    }
    return prev_state;
}

void LampMonitor::invalidateButtonState()
{
    if (auto const lamp_field_monitor = getLampFieldMonitor()) {
        lamp_field_monitor->invalidateButtonState();
    }
}

std::shared_ptr<LampFieldMonitor> LampMonitor::getLampFieldMonitor()
{
    std::lock_guard const lock(lamp_field_monitor_mut_);
    return lamp_field_monitor_.lock();
}

void LampMonitor::setLampFieldMonitor(std::weak_ptr<LampFieldMonitor> const &lamp_field_monitor)
{
    std::lock_guard const lock(lamp_field_monitor_mut_);
    lamp_field_monitor_ = lamp_field_monitor;
}
