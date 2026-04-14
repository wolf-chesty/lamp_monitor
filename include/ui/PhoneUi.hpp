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

/// @class PhoneUI
/// @namespace ui
///
/// @brief Provides an interface for objects that can generate XML data specific to hardware deskphones.
class PhoneUI {
public:
    PhoneUI() = default;
    virtual ~PhoneUI() = default;

    /// @brief Invoked whenever the button state for a lamp field is updated.
    ///
    /// @param buttons Collection of buttons that have had their state changed.
    void update(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

    /// @brief Can be invoked by users of this object to get a string representation of the phones UI.
    ///
    /// @return String that can set a deskphones appearance.
    std::string toString();

    /// @brief Indicates whether the phone state is critical and should be forced onto the phone.
    ///
    /// @return \c true if the phone state is considered critical and the update should be pushed to the phone.
    bool isCritical();

    /// @brief Invoked by users of this object to populate a PJSIP notification action that the deskphone will
    ///        recognize.
    ///
    /// @param action PJSIP notification action to populate with deskphone specific data.
    virtual void initialize(cpp_ami::action::PJSIPNotify &action) = 0;

protected:
    /// @brief Provides an atomic operation to return the current cached phone state.
    ///
    /// @return Pointer to the current phone UI state.
    std::shared_ptr<PhoneUIState> getPhoneState();

    /// @brief Provides an atomic operation to set the current phone state.
    ///
    /// @return Pointer to the new phone UI state.
    void setPhoneState(std::shared_ptr<PhoneUIState> const &state);

    /// @brief Creates the new phone UI state from the buttons states and initial critical state.
    ///
    /// @param button Button to create the new phone state XML from.
    /// @param critical Overrides the initial critical state of the UI state.
    ///
    /// @return XML state and critical flag.
    virtual std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::shared_ptr<button_state::PhoneButton> const &button, bool critical) = 0;

    /// @brief Creates the new phone UI state from the buttons states and initial critical state.
    ///
    /// @param buttons Buttons to create the new phone state XML from.
    /// @param critical Overrides the initial critical state of the UI state.
    ///
    /// @return XML state and critical flag.
    virtual std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical) = 0;

private:
    std::shared_ptr<PhoneUIState> cached_button_state_; ///< Current phone UI state.
    std::shared_mutex cached_button_state_mut_;         ///< Mutex on phone UI state.
};

} // namespace ui

#endif
