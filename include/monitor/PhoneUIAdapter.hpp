// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_LAMP_FIELD_ADAPTER_HPP
#define MONITOR_LAMP_FIELD_ADAPTER_HPP

#include "button_state/PhoneButton.hpp"
#include "monitor/PhoneUIState.hpp"
#include <c++ami/action/PjsipNotify.hpp>
#include <memory>
#include <pugixml.hpp>
#include <shared_mutex>
#include <vector>

namespace monitor {

class PhoneUIAdapter {
public:
    PhoneUIAdapter() = default;
    virtual ~PhoneUIAdapter() = default;

    void update(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

    std::string toString();

    bool isCritical();

    void initialize(cpp_ami::action::PJSIPNotify &action);

    static pugi::xml_document
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

protected:
    std::shared_ptr<PhoneUIState> getPhoneState();
    void setPhoneState(std::shared_ptr<PhoneUIState> const &state);

private:
    static char const *toColorString(button_state::PhoneButton::Color const color);
    static char const *toButtonStateString(button_state::PhoneButton const &button);

    static pugi::xml_document
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool &critical);

    std::shared_ptr<PhoneUIState> cached_button_state_;
    std::shared_mutex cached_button_state_mut_;
};

} // namespace monitor

#endif
