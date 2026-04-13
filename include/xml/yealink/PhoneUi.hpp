// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_PHONE_UI_HPP
#define XML_YEALINK_PHONE_UI_HPP

#include "ui/PhoneUi.hpp"

namespace xml::yealink {

class PhoneUI : public ui::PhoneUI {
public:
    PhoneUI() = default;
    ~PhoneUI() override = default;

    void initialize(cpp_ami::action::PJSIPNotify &action) override;

    static std::string createYealinkXMLString(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

protected:
    std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons,
                            bool critical) override;

private:
    static char const *toColorString(button_state::PhoneButton::Color const color);
    static char const *toButtonStateString(button_state::PhoneButton const &button);

    static std::pair<pugi::xml_document, bool>
        createYealinkXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical);
};

} // namespace xml::yealink

#endif
