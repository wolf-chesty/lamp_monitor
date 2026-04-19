// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ui/HttpParkButton.hpp"

#include "xml/yealink/CallParkMenu.hpp"
#include <cassert>

using namespace ui;

std::shared_ptr<HTTPParkButton> HTTPParkButton::create(std::string_view type,
                                                       std::shared_ptr<cpp_ami::Connection> const &conn,
                                                       std::string const &parking_lot,
                                                       std::string const &parked_call_info_uri)
{
    assert(!type.empty());

    if (type == "yealink") {
        return std::make_shared<xml::yealink::CallParkMenu>(conn, parking_lot, parked_call_info_uri);
    }
    return nullptr;
}
