// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "asterisk/EventHandler.hpp"

using namespace asterisk;

EventHandler::EventHandler(std::shared_ptr<cpp_ami::Connection> conn)
    : io_conn_(std::move(conn))
{
}

std::shared_ptr<cpp_ami::Connection> EventHandler::getConnection()
{
    return io_conn_;
}
