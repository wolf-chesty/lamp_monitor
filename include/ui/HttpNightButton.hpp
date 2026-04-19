// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_HTTP_NIGHT_BUTTON_HPP
#define UI_HTTP_NIGHT_BUTTON_HPP

#include "asterisk/NightEventHandler.hpp"
#include "button_state/PhoneButton.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace ui {

class HTTPNightButton {
public:
    HTTPNightButton() = default;
    virtual ~HTTPNightButton() = default;

    static std::shared_ptr<HTTPNightButton> create(std::string_view type,
                                                   std::shared_ptr<button_state::PhoneButton> const &button,
                                                   std::shared_ptr<cpp_ami::Connection> const &conn,
                                                   std::string const &device);

    virtual std::string httpPushButton() = 0;
    virtual std::string getContentType() = 0;
};

} // namespace ui

#endif