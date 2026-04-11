// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_MONITOR_HPP
#define LAMP_MONITOR_HPP

#include <memory>

#include <c++ami/Connection.hpp>
#include <pugixml.hpp>

class LampFieldMonitor;

/// @class LampMonitor
///
/// @brief Base class for objects that manage button lamps on the phone.
class LampMonitor : public std::enable_shared_from_this<LampMonitor> {
public:
    explicit LampMonitor(std::shared_ptr<cpp_ami::Connection> io_conn, uint8_t button_id, bool button_on = false);
    virtual ~LampMonitor() = default;

    void setLampFieldMonitor(std::weak_ptr<LampFieldMonitor> const &lamp_field_monitor);

    uint8_t getButtonId() const;

    bool getButtonOn() const;
    bool setButtonOn(bool button_on);

    virtual bool needsBeep() const = 0;
    virtual void getButtonState(pugi::xml_node button_state_node) const = 0;

protected:
    std::shared_ptr<cpp_ami::Connection> getAMIConnection() const;

    void invalidateButtonState();

private:
    std::shared_ptr<LampFieldMonitor> getLampFieldMonitor();

    std::weak_ptr<LampFieldMonitor> lamp_field_monitor_; ///< Pointer to \c LampFieldMonitor that this lamp belongs to.
    std::mutex lamp_field_monitor_mut_;                  ///< Controlling mutex for \c LampFieldMonitor pointer.
    std::shared_ptr<cpp_ami::Connection> io_conn_;       ///< AMI connection for input/output.
    uint8_t button_id_;                                  ///< ID for this lamp.
    std::atomic<bool> button_on_{false};                 ///< Button state.
};

#endif
