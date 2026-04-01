// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PARKED_CALL_MONITOR_HPP
#define PARKED_CALL_MONITOR_HPP

#include "lamp_monitor/LampMonitor.hpp"

#include <c++ami/util/KeyValDict.hpp>
#include <string>
#include <unordered_set>

/// @class ParkedCallMonitor
///
/// @brief Monitors the parked calls of the Asterisk system and creates a parking list for the phone UI.
class ParkedCallMonitor : public LampMonitor {
public:
    ParkedCallMonitor() = delete;
    ParkedCallMonitor(ParkedCallMonitor const &) = delete;
    ParkedCallMonitor(ParkedCallMonitor &&) noexcept = delete;
    explicit ParkedCallMonitor(std::shared_ptr<cpp_ami::Connection> const &io_conn, uint8_t button_id,
                               std::string parked_call_info_uri);
    ~ParkedCallMonitor() override;

    ParkedCallMonitor &operator=(ParkedCallMonitor const &) = delete;
    ParkedCallMonitor &operator=(ParkedCallMonitor &&) noexcept = delete;

    static std::string createMessageXML(bool beep, uint8_t timeout, std::string const &title, std::string const &text);

    std::string getParkedCallMenu() const;
    std::string getParkedCallDetails(std::string const &park_exten) const;

    // LampMonitor interface
    bool needsBeep() const override;
    void getButtonState(pugi::xml_node button_state_node) const override;

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::string createParkedCallMenu(cpp_ami::reaction::EventList const &parked_call_list) const;

    static std::string createNoParkedCallMessage();

    std::string parked_call_info_uri_;                                  ///< URI for parked call info.
    std::unordered_set<std::string> parked_extens_;                     ///< Collection of active parking extensions.
    std::atomic<size_t> parked_call_count_;                             ///< Number of parked calls.
    cpp_ami::Connection::event_callback_key_t parked_call_callback_id_; ///< AMI callback ID.
};

#endif
