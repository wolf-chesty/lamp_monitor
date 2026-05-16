// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/ParkButton.hpp"

#include "bridge/yealink/ParkButton.hpp"
#include <cassert>

using namespace bridge;

std::shared_ptr<ParkButton> ParkButton::create(std::string_view type,
                                                       std::shared_ptr<cpp_ami::Connection> const &conn,
                                                       std::string const &parking_lot,
                                                       std::string const &parked_call_info_uri)
{
    assert(!type.empty());

    if (type == "yealink") {
        return std::make_shared<bridge::yealink::ParkButton>(conn, parking_lot, parked_call_info_uri);
    }
    return nullptr;
}
