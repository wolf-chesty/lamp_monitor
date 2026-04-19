// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "button_state/ButtonPlan.hpp"

#include <cassert>
#include <execution>
#include <syslog.h>

using namespace button_state;

ButtonPlan::ButtonPlan(std::string name, std::shared_ptr<ui::PhoneEventDispatcher> ami_bridge)
    : name_(std::move(name))
    , ami_bridge_(std::move(ami_bridge))
{
}

std::shared_ptr<ButtonPlan> ButtonPlan::create(YAML::Node const &config,
                                               std::shared_ptr<ui::PhoneEventDispatcher> const &ami_bridge)
{
    auto const &name = config["name"].as<std::string>();
    auto const button_plan = std::make_shared<ButtonPlan>(name, ami_bridge);

    for (auto const &button_cfg : config["buttons"]) {
        auto const button = PhoneButton::create(button_cfg, button_plan);
        assert(button);
        button_plan->addButton(button->getButtonID(), button);
    }

    return button_plan;
}

std::string const &ButtonPlan::getName() const
{
    return name_;
}

void ButtonPlan::removeButton(uint16_t const button_id)
{
    std::lock_guard const lock(buttons_mut_);
    buttons_.erase(button_id);
}

std::vector<std::shared_ptr<PhoneButton>> ButtonPlan::getButtons()
{
    std::shared_lock const lock(buttons_mut_);
    std::vector<std::shared_ptr<PhoneButton>> buttons;
    std::ranges::transform(buttons_.begin(), buttons_.end(), std::back_inserter(buttons),
                           [](auto const &itr) -> std::shared_ptr<PhoneButton> { return itr.second; });
    return buttons;
}

std::shared_ptr<PhoneButton> ButtonPlan::getButton(uint16_t const button_id)
{
    std::shared_lock const lock(buttons_mut_);
    auto const itr = buttons_.find(button_id);
    return itr != buttons_.end() ? itr->second : nullptr;
}

void ButtonPlan::invalidate([[maybe_unused]] uint16_t const button_id)
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
                      ami_bridge_->dispatch(action);
                  });
}

bool ButtonPlan::registerUI(std::string const &ui_name, std::shared_ptr<ui::PhoneUI> const &ui)
{
    std::lock_guard const lock(phone_uis_mut_);
    auto const &[itr, success] = phone_uis_.emplace(ui_name, ui);
    if (success) {
        ui->update(getButtons());
    }
    return success;
}

void ButtonPlan::unregisterUI(std::string const &ui_name)
{
    std::lock_guard const lock(phone_uis_mut_);
    phone_uis_.erase(ui_name);
}

std::shared_ptr<ui::PhoneUI> ButtonPlan::getPhoneUI(std::string const &ui_type)
{
    std::shared_lock const lock(phone_uis_mut_);
    auto const itr = phone_uis_.find(ui_type);
    return itr != phone_uis_.end() ? itr->second : nullptr;
}

void ButtonPlan::addButton(uint16_t const button_id, std::shared_ptr<PhoneButton> const &button)
{
    std::lock_guard const lock(buttons_mut_);
    if (auto const [_, added] = buttons_.emplace(button_id, button); !added) {
        assert(added);
        syslog(LOG_WARNING, "Unable to add button %ud to plan '%s'", button_id, name_.c_str());
    }
}

void ButtonPlan::addEventHandler(uint16_t id, std::shared_ptr<asterisk::EventHandler> const &event_handler)
{
    std::lock_guard const lock(event_handlers_mut_);
    auto const [_, success] = event_handlers_.emplace(id, event_handler);
    assert(success);
}

std::shared_ptr<asterisk::EventHandler> ButtonPlan::getEventHandler(uint16_t const id)
{
    std::shared_lock const lock(event_handlers_mut_);
    auto const &itr = event_handlers_.find(id);
    return itr != event_handlers_.end() ? itr->second : nullptr;
}
