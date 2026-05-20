// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_YEALINK_PHONE_UI_HPP
#define BRIDGE_YEALINK_PHONE_UI_HPP

#include "bridge/HTTPStateProvider.hpp"
#include "bridge/PhoneUi.hpp"

namespace bridge::yealink {

/// @class PhoneUI
/// @namespace bridge::yealink
///
/// @brief Class generates XML browser code for Yealink deskphones.
class PhoneUI
    : public bridge::HTTPStateProvider
    , public bridge::PhoneUI {
public:
    explicit PhoneUI(std::string name, std::shared_ptr<bridge::AoRProvider> provider);
    ~PhoneUI() override = default;

    /// @brief Populates \c action with PJSIP notification text compatible with Yealink deskphones.
    ///
    /// @param action Action to populate with a Yealink compatible payload.
    void initialize(cpp_ami::action::PJSIPNotify &action) override;

    /// @brief Pushes the HTTP button, returning HTTP body with result.
    ///
    /// @return HTTP result body.
    std::string getHTTPState() override;

    /// @brief Returns the HTTP content type for text created by this object.
    ///
    /// @return HTTP content type for text created by this object.
    std::string getContentType() override;

    /// @brief Creates an XML browser text body compatible with Yealink deskphones.
    ///
    /// @param button Button to generate XML text for.
    ///
    /// @return String containing XML browser code compatible with Yealink deskphones.
    static std::string createYealinkXMLString(std::shared_ptr<button_state::PhoneButton> const button);

    /// @brief Creates an XML browser text body compatible with Yealink deskphones.
    ///
    /// @param buttons Buttons to generate XML text for.
    ///
    /// @return String containing XML browser code compatible with Yealink deskphones.
    static std::string createYealinkXMLString(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

protected:
    /// @brief Generates XML code for the phone UI's current state.
    ///
    /// @param button Button to generate state XML for.
    /// @param critical Set to \c true if you wish to force the phone state to be critical.
    ///
    /// @return XML representing the phones current state and bool flag indicating if state is critical.
    std::pair<pugi::xml_document, bool> createPhoneStateXML(std::shared_ptr<button_state::PhoneButton> const &button,
                                                            bool critical) override;

    /// @brief Generates XML code for the phone UI's current state.
    ///
    /// @param buttons Buttons to generate state XML for.
    /// @param critical Set to \c true if you wish to force the phone state to be critical.
    ///
    /// @return XML representing the phones current state and bool flag indicating if state is critical.
    std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons,
                            bool critical) override;

private:
    /// @brief Converts \c color into XML text for use with the phone state XML.
    ///
    /// @param color Value to convert to string value.
    ///
    /// @return String representation of \c color.
    static char const *toColorString(button_state::PhoneButton::Color const color);

    /// @brief Converts \c button to string compatible with XML.
    ///
    /// @param button Button to convert to string.
    ///
    /// @eturn String representation of \c button.
    static char const *toButtonStateString(button_state::PhoneButton const &button);

    /// @brief Creates Yealink XML browser code.
    ///
    /// @param button Button to create XML browser code from.
    /// @param critical Set to \c true to force the XML browser to wake the phone screen.
    ///
    /// @return Phone XML browser code and critical state of state.
    static std::pair<pugi::xml_document, bool>
        createYealinkXML(std::shared_ptr<button_state::PhoneButton> const &button, bool critical);

    /// @brief Creates Yealink XML browser code.
    ///
    /// @param buttons Buttons to create XML browser code from.
    /// @param critical Set to \c true to force the XML browser to wake the phone screen.
    ///
    /// @return Phone XML browser code and critical state of state.
    static std::pair<pugi::xml_document, bool>
        createYealinkXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical);
};

} // namespace bridge::yealink

#endif
