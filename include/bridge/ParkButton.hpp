// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_PARK_BUTTON_HPP
#define BRIDGE_PARK_BUTTON_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace bridge {

class ParkButton {
public:
    ParkButton() = default;
    virtual ~ParkButton() = default;

    static std::shared_ptr<ParkButton> create(std::string_view type, std::shared_ptr<cpp_ami::Connection> const &conn,
                                              std::string const &parking_lot, std::string const &parked_call_info_uri);

    virtual std::string pushButton() const = 0;
    virtual std::string pushButton(std::string const &exten) const = 0;
    virtual std::string displayErrorMessage(std::string const &title, std::string const &text) const = 0;
    virtual std::string getContentType() const = 0;
};

} // namespace bridge

#endif
