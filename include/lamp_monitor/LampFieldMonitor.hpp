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
#include <unordered_set>

class LampFieldMonitor : public std::enable_shared_from_this<LampFieldMonitor> {
public:
    LampFieldMonitor() = delete;
    LampFieldMonitor(LampFieldMonitor const &) = delete;
    LampFieldMonitor(LampFieldMonitor &&) = delete;
    explicit LampFieldMonitor(std::shared_ptr<cpp_ami::Connection> io_conn);
    virtual ~LampFieldMonitor();

    void addLamp(std::shared_ptr<LampMonitor> const &lamp);

    void clearPhoneCache();
    void addPhone(std::string phone);

    static std::string getButtonStateXML(std::vector<std::shared_ptr<LampMonitor>> const &lamps, bool beep = false);

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    void publishButtonState(std::string const &aor);
    std::string getButtonStateXML();

    std::vector<std::shared_ptr<LampMonitor>> lamps_;
    std::mutex lamps_mut_;
    std::shared_ptr<cpp_ami::Connection> io_conn_;
    cpp_ami::Connection::event_callback_key_t ami_callback_id_;
    std::mutex desk_phones_mut_;
    std::unordered_set<std::string> desk_phones_;
};

#endif