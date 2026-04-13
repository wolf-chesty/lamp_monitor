// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_PHONE_UI_ADAPTER_HPP
#define MONITOR_PHONE_UI_ADAPTER_HPP

#include "button_state/PhoneButton.hpp"
#include "monitor/PhoneUiState.hpp"
#include <c++ami/action/PjsipNotify.hpp>
#include <memory>
#include <pugixml.hpp>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace monitor {

class PhoneUIAdapter {
public:
    PhoneUIAdapter() = default;
    virtual ~PhoneUIAdapter() = default;

    void update(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

    std::string toString();

    bool isCritical();

    virtual void initialize(cpp_ami::action::PJSIPNotify &action) = 0;

protected:
    std::shared_ptr<PhoneUIState> getPhoneState();
    void setPhoneState(std::shared_ptr<PhoneUIState> const &state);

    virtual std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical) = 0;

private:
    std::shared_ptr<PhoneUIState> cached_button_state_;
    std::shared_mutex cached_button_state_mut_;
};

} // namespace monitor

#endif
