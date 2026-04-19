// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_HTTP_STATE_BUTTON_HPP
#define UI_HTTP_STATE_BUTTON_HPP

#include <string>

namespace ui {

class HTTPStateButton {
public:
    HTTPStateButton() = default;
    virtual ~HTTPStateButton() = default;

    virtual std::string httpPushButton() = 0;
    virtual std::string getContentType() = 0;
};

}

#endif
