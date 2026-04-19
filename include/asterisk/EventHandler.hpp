// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_EVENT_HANDLER_HPP
#define AST_BRIDGE_EVENT_HANDLER_HPP

#include <c++ami/Connection.hpp>

namespace asterisk {

class EventHandler {
public:
    enum class EventType { Night, Park, Register };

public:
    explicit EventHandler(std::shared_ptr<cpp_ami::Connection> io_conn);
    virtual ~EventHandler() = default;

    virtual EventType getType() const = 0;

protected:
    std::shared_ptr<cpp_ami::Connection> getConnection();

private:
    std::shared_ptr<cpp_ami::Connection> io_conn_; ///< Asterisk AMI connection.
};

} // namespace asterisk

#endif
