// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef NIGHT_LAMP_MONITOR_HPP
#define NIGHT_LAMP_MONITOR_HPP

#include "lamp_monitor/NightLampState.hpp"

#include <atomic>
#include <c++ami/util/KeyValDict.hpp>
#include <string>

class NightLampMonitor : public NightLampState {
public:
    NightLampMonitor() = delete;
    NightLampMonitor(NightLampMonitor const &) = delete;
    NightLampMonitor(NightLampMonitor &&) = delete;
    explicit NightLampMonitor(std::shared_ptr<cpp_ami::Connection> const &io_conn, uint8_t button_id,
                              std::string night_exten, std::string context, std::string device);
    ~NightLampMonitor() override;

    NightLampMonitor &operator=(NightLampMonitor const &) = delete;
    NightLampMonitor &operator=(NightLampMonitor &&) = delete;

    std::string resetNightState();

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    void updateLampState(std::string_view device_state);

    std::string park_exten_;
    std::string context_;
    std::string device_;
    cpp_ami::Connection::event_callback_key_t ami_callback_id_;
};

#endif
