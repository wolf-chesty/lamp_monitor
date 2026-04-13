// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_PHONE_UI_HPP
#define UI_PHONE_UI_HPP

#include "button_state/PhoneButton.hpp"
#include "PhoneUiState.hpp"
#include <c++ami/action/PjsipNotify.hpp>
#include <memory>
#include <pugixml.hpp>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace ui {

class PhoneUI {
public:
    PhoneUI() = default;
    virtual ~PhoneUI() = default;

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
