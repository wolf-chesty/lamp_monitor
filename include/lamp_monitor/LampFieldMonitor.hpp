// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_FIELD_MONITOR_HPP
#define LAMP_FIELD_MONITOR_HPP

#include "lamp_monitor/LampMonitor.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

class LampFieldMonitor : public std::enable_shared_from_this<LampFieldMonitor> {
public:
    LampFieldMonitor() = delete;
    LampFieldMonitor(LampFieldMonitor const &) = delete;
    LampFieldMonitor(LampFieldMonitor &&) = delete;
    explicit LampFieldMonitor(std::shared_ptr<cpp_ami::Connection> io_conn);
    virtual ~LampFieldMonitor();

    void addLamp(std::shared_ptr<LampMonitor> const &lamp);

    void invalidateButtonState();

    static std::string getButtonStateXML(std::vector<std::shared_ptr<LampMonitor>> const &lamps, bool beep = false);

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::string getButtonStateXML();

    std::string const &setCachedButtonStateXML(std::string button_state_xml);
    std::string const &getCachedButtonStateXML();

    void publishButtonState(std::string const &button_state_xml) const;

    void startWorkThread();
    void stopWorkThread();
    void workThread();

    std::shared_ptr<cpp_ami::Connection> io_conn_;
    cpp_ami::Connection::event_callback_key_t ami_callback_id_;

    std::vector<std::shared_ptr<LampMonitor>> lamps_; ///< Collection of objects that monitor phone lamp state.
    std::mutex lamps_mut_;                            ///< Mutex on \c LampMonitor collection.

    std::thread button_state_thread_;
    std::atomic<bool> button_state_thread_run_{};
    std::atomic<bool> button_state_valid_{true};
    std::condition_variable button_state_cv_;

    std::mutex button_state_xml_mut_;
    std::string button_state_xml_;
};

#endif