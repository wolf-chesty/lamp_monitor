// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_NIGHT_BUTTON_HPP
#define MONITOR_NIGHT_BUTTON_HPP

#include "lamp_state/PhoneButton.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>

namespace monitor {

class NightButton {
public:
    explicit NightButton(std::shared_ptr<lamp_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string night_exten, std::string context,
                         std::string device);
    virtual ~NightButton();

protected:
    std::shared_ptr<cpp_ami::Connection> getAMIConnection();
    std::string const &getDevice();
    std::shared_ptr<lamp_state::PhoneButton> getPhoneButton();

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::shared_ptr<lamp_state::PhoneButton> phone_button_;
    std::shared_ptr<cpp_ami::Connection> io_conn_;
    std::string night_exten_;
    std::string context_;
    std::string device_;
    cpp_ami::Connection::event_callback_key_t ami_callback_id_;
};

} // namespace monitor

#endif
