// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_MONITOR_HPP
#define LAMP_MONITOR_HPP

#include <memory>

#include <c++ami/Connection.hpp>
#include <pugixml.hpp>

/// @class LampMonitor
///
/// @brief Base class for objects that manage button lamps on the phone.
class LampMonitor : public std::enable_shared_from_this<LampMonitor> {
public:
    LampMonitor(std::shared_ptr<cpp_ami::Connection> ami_conn, uint8_t button_id);
    LampMonitor(LampMonitor const &) = default;
    LampMonitor(LampMonitor &&) = default;
    virtual ~LampMonitor() = default;

    virtual bool needsBeep() const = 0;
    virtual void getButtonState(pugi::xml_node button_state_node) const = 0;

protected:
    std::shared_ptr<cpp_ami::Connection> getAMIConnection() const;
    uint8_t getButtonId() const;

    std::string getButtonStateXML(bool const beep);
    void sendButtonStateToPhones(bool const beep);

private:
    std::shared_ptr<cpp_ami::Connection> ami_conn_;
    uint8_t button_id_;
};

#endif
