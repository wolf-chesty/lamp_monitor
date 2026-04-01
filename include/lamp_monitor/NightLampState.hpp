// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/LampMonitor.hpp"

#ifndef NIGHT_LAMP_STATE_HPP
#define NIGHT_LAMP_STATE_HPP

class NightLampState : public LampMonitor {
public:
    explicit NightLampState(std::shared_ptr<cpp_ami::Connection> io_conn, uint8_t button_id, bool button_on);
    NightLampState(NightLampState const &) = delete;
    NightLampState(NightLampState &&) = delete;
    ~NightLampState() override = default;

    NightLampState &operator=(NightLampState const &) = delete;
    NightLampState &operator=(NightLampState &&) = delete;

    // LampMonitor interface
    bool needsBeep() const override;
    void getButtonState(pugi::xml_node button_state_node) const override;

protected:
    bool getButtonState();
    bool setButtonState(bool button_on);

private:
    std::atomic<bool> button_on_{false};
};

#endif
