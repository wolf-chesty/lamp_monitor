// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_HTTP_NIGHT_BUTTON_HPP
#define XML_YEALINK_HTTP_NIGHT_BUTTON_HPP

#include <button_state/PhoneButton.hpp>
#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace xml::yealink {

class HTTPNightButton {
public:
    explicit HTTPNightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                         std::shared_ptr<cpp_ami::Connection> io_conn, std::string device);
    ~HTTPNightButton() = default;

    std::string pushButton();

private:
    std::shared_ptr<button_state::PhoneButton> button_;
    std::shared_ptr<cpp_ami::Connection> io_conn_;
    std::string device_;
};

} // namespace xml::yealink

#endif
