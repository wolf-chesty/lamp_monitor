// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_YEALINK_NIGHT_BUTTON_HPP
#define MONITOR_YEALINK_NIGHT_BUTTON_HPP

#include "monitor/NightButton.hpp"

namespace monitor::yealink {

class NightButton : public monitor::NightButton {
public:
    NightButton(std::shared_ptr<lamp_state::PhoneButton> phone_button, std::shared_ptr<cpp_ami::Connection> io_conn,
                std::string night_exten, std::string context, std::string device);
    ~NightButton() override = default;

    std::string resetNightButtonState();
};

} // namespace monitor::yealink

#endif
