// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "button_state/LampField.hpp"

#include <cassert>
#include <execution>

using namespace button_state;

LampField::LampField(std::shared_ptr<ui::PhoneBridge> phone_bridge)
    : phone_bridge_(std::move(phone_bridge))
{
    assert(phone_bridge_);
}

/// This function will create new phone buttons in the case where a phone button with \c button_id doesn't exist. In the
/// event that a button object exists with a button id of \c button_id, the existing button is returned.
std::shared_ptr<PhoneButton> LampField::getButton(uint16_t const button_id, PhoneButton::Color const color,
                                                  bool const flash, bool const critical, bool const on)
{
    std::lock_guard const lock(buttons_mut_);

    auto const it = buttons_.find(button_id);
    if (it == buttons_.end()) {
        auto button = std::make_shared<PhoneButton>(shared_from_this(), button_id, color, flash, critical, on);
        buttons_.emplace(button_id, button);
        return button;
    }
    return it->second;
}

void LampField::removeButton(uint16_t const button_id)
{
    std::lock_guard const lock(buttons_mut_);
    buttons_.erase(button_id);
}

std::vector<std::shared_ptr<PhoneButton>> LampField::getButtons()
{
    std::shared_lock const lock(buttons_mut_);
    std::vector<std::shared_ptr<PhoneButton>> buttons;
    std::ranges::transform(buttons_.begin(), buttons_.end(), std::back_inserter(buttons),
                           [](auto const &itr) -> std::shared_ptr<PhoneButton> { return itr.second; });
    return buttons;
}

std::shared_ptr<PhoneButton> LampField::getButton(uint16_t const button_id)
{
    std::shared_lock const lock(buttons_mut_);
    auto const itr = buttons_.find(button_id);
    return itr != buttons_.end() ? itr->second : nullptr;
}

void LampField::invalidate([[maybe_unused]] uint16_t const button_id)
{
    std::shared_lock const lock(phone_uis_mut_);
    std::for_each(std::execution::par, phone_uis_.begin(), phone_uis_.end(),
                  [this, buttons = getButtons()](auto const &itr) -> void {
                      auto const ui = itr.second;
                      // Update the UI state
                      ui->update(buttons);
                      // Publish the UI state to the physical deskphones
                      cpp_ami::action::PJSIPNotify action;
                      ui->initialize(action);
                      phone_bridge_->dispatch(itr.first, std::move(action));
                  });
}

void LampField::registerUI(std::string const &ui_name, std::shared_ptr<ui::PhoneUI> const &ui)
{
    std::lock_guard const lock(phone_uis_mut_);
    phone_uis_.emplace(ui_name, ui);
}

void LampField::unregisterUI(std::string const &ui_name)
{
    std::lock_guard const lock(phone_uis_mut_);
    phone_uis_.erase(ui_name);
}

std::shared_ptr<ui::PhoneUI> LampField::getPhoneUI(std::string const &ui_name)
{
    std::shared_lock const lock(phone_uis_mut_);
    if (auto const itr = phone_uis_.find(ui_name); itr != phone_uis_.end()) {
        return itr->second;
    }
    return nullptr;
}
