// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PARKED_CALL_MONITOR_HPP
#define PARKED_CALL_MONITOR_HPP

#include "LampMonitor.hpp"

#include <c++ami/util/KeyValDict.hpp>
#include <string>
#include <unordered_set>

/// @class ParkedCallMonitor
///
/// @brief Monitors parked calls on the Asterisk system and creates a parking list for the phone UI.
///
/// This object will monitor the Asterisk server for parked calls and update the parking lamp on the deskphones. The
/// object can also create deskphone interactive XML browser menus to list parked calls and their details.
class ParkedCallMonitor : public LampMonitor {
public:
    explicit ParkedCallMonitor(std::shared_ptr<cpp_ami::Connection> const &io_conn, uint8_t button_id
                               );
    ~ParkedCallMonitor() override;

    // LampMonitor interface
    bool needsBeep() const override;
    void getButtonState(pugi::xml_node button_state_node) const override;

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    std::unordered_set<std::string> parked_extens_;                     ///< Collection of active parking extensions.
    std::atomic<size_t> parked_call_count_;                             ///< Number of parked calls.
    cpp_ami::Connection::event_callback_key_t parked_call_callback_id_; ///< AMI callback ID.
};

#endif
