// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_HTTP_PARK_BUTTON_HPP
#define UI_HTTP_PARK_BUTTON_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace ui {

class HTTPParkButton {
public:
    HTTPParkButton() = default;
    virtual ~HTTPParkButton() = default;

    static std::shared_ptr<HTTPParkButton> create(std::string_view type,
                                                  std::shared_ptr<cpp_ami::Connection> const &conn,
                                                  std::string const &parking_lot,
                                                  std::string const &parked_call_info_uri);

    virtual std::string httpPushButton() const = 0;
    virtual std::string httpPushButton(std::string const &exten) const = 0;
    virtual std::string displayErrorMessage(std::string const &title, std::string const &text) const = 0;
    virtual std::string getContentType() const = 0;
};

} // namespace ui

#endif
