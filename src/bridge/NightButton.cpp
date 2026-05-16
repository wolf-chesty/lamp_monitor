// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/NightButton.hpp"

#include "bridge/yealink/NightButton.hpp"
#include <c++ami/action/Setvar.hpp>
#include <c++ami/util/ScopeGuard.hpp>
#include <cassert>
#include <fmt/core.h>
#include <syslog.h>

using namespace bridge;

NightButton::NightButton(std::shared_ptr<button_state::PhoneButton> phone_button,
                                 std::shared_ptr<cpp_ami::Connection> io_conn, std::string device)
    : button_(std::move(phone_button))
    , io_conn_(std::move(io_conn))
    , device_(std::move(device))
{
    assert(button_);
    assert(io_conn_);
}

std::shared_ptr<NightButton> NightButton::create(std::string_view type,
                                                         std::shared_ptr<button_state::PhoneButton> const &button,
                                                         std::shared_ptr<cpp_ami::Connection> const &conn,
                                                         std::string const &device)
{
    assert(!type.empty());
    assert(!device.empty());

    if (type == "yealink") {
        return std::make_shared<bridge::yealink::NightButton>(button, conn, device);
    }
    assert(false);
    return nullptr;
}

std::string NightButton::pushButton()
{
    auto const button_on = !button_->isOn();

    // Update device state on Asterisk server
    cpp_ami::util::ScopeGuard const post_action([io_conn = io_conn_, device = device_, button_on]() -> void {
        assert(io_conn);
        cpp_ami::action::Setvar action;
        action["Variable"] = fmt::format("DEVICE_STATE({})", device);
        action["Value"] = button_on ? "INUSE" : "NOT_INUSE";
        syslog(LOG_DEBUG, "HTTPNightButton::pushButton() : Posting AMI action \"%s\"", action.toString().c_str());
        io_conn->asyncInvoke(action);
    });

    return pushButton(button_->clone(), button_on);
}
