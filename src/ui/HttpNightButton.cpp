// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ui/HttpNightButton.hpp"

#include "bridge/yealink/HttpNightButton.hpp"
#include <cassert>

using namespace ui;

std::shared_ptr<HTTPNightButton> HTTPNightButton::create(std::string_view type,
                                                         std::shared_ptr<button_state::PhoneButton> const &button,
                                                         std::shared_ptr<cpp_ami::Connection> const &conn,
                                                         std::string const &hint)
{
    assert(!type.empty());
    assert(!hint.empty());

    if (type == "yealink") {
        return std::make_shared<bridge::yealink::HTTPNightButton>(button, conn, hint);
    }
    return nullptr;
}
