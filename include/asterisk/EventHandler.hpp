// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_EVENT_HANDLER_HPP
#define AST_BRIDGE_EVENT_HANDLER_HPP

#include <c++ami/Connection.hpp>

namespace asterisk {

/// @class EventHandler
/// @namespace asterisk
///
/// @brief Provides a base class for application Asterisk AMI event handlers.
///
/// This class provides a base class so that a collection of AMI event handlers can be kept during the runtime of the
/// application.
class EventHandler {
public:
    /// @enum EventType
    ///
    /// @brief Event type being monitored.
    enum class EventType { Night, Park, Register };

public:
    explicit EventHandler(std::shared_ptr<cpp_ami::Connection> io_conn);
    virtual ~EventHandler() = default;

    /// @brief Returns the type of event handler.
    ///
    /// @return Event type being monitored by object.
    virtual EventType getType() const = 0;

protected:
    /// @brief Returns the Asterisk AMI connection object for this class.
    ///
    /// @return Pointer to an Asterisk AMI connection.
    std::shared_ptr<cpp_ami::Connection> getConnection();

private:
    std::shared_ptr<cpp_ami::Connection> io_conn_; ///< Asterisk AMI connection.
};

} // namespace asterisk

#endif
