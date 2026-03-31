// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_FIELD_MONITOR_HPP
#define LAMP_FIELD_MONITOR_HPP

#include "lamp_monitor/LampMonitor.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <mutex>
#include <vector>

class LampFieldMonitor {
public:
    LampFieldMonitor() = delete;
    LampFieldMonitor(LampFieldMonitor const &) = delete;
    LampFieldMonitor(LampFieldMonitor &&) = delete;
    explicit LampFieldMonitor(std::shared_ptr<cpp_ami::Connection> ami_conn);
    ~LampFieldMonitor();

    void addLamp(std::shared_ptr<LampMonitor> const &lamp);

    static std::string getButtonStateXML(std::vector<std::shared_ptr<LampMonitor>> const &lamps, bool beep = false);

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    void publishButtonState(std::string const &aor);
    std::string getButtonStateXML();

    std::vector<std::shared_ptr<LampMonitor>> lamps_;
    std::mutex lamps_mut_;
    std::shared_ptr<cpp_ami::Connection> ami_conn_;
    cpp_ami::Connection::event_callback_key_t ami_callback_id_;
};

#endif