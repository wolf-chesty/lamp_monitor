// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_PHONE_UI_HPP
#define BRIDGE_PHONE_UI_HPP

#include "bridge/AorProvider.hpp"
#include "bridge/PhoneUiState.hpp"
#include "button_state/PhoneButton.hpp"
#include "c++ami/Connection.hpp"
#include <c++ami/action/PjsipNotify.hpp>
#include <memory>
#include <pugixml.hpp>
#include <shared_mutex>
#include <unordered_set>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace bridge {

/// @class PhoneUI
/// @namespace bridge
///
/// @brief Provides an interface for objects that can generate XML data to control the UI of hardware deskphones.
class PhoneUI {
public:
    explicit PhoneUI(std::string name, std::shared_ptr<bridge::AoRProvider> provider);
    virtual ~PhoneUI();

    /// @brief Creates a new object using parameters in \c config.
    ///
    /// @param config Configuration parameters.
    ///
    /// @return Pointer to new object and its name.
    static std::shared_ptr<PhoneUI> create(YAML::Node const &config,
                                           std::shared_ptr<cpp_ami::Connection> const &io_conn);

    /// @brief Invoked whenever the button state for a lamp field is updated.
    ///
    /// @param buttons Collection of buttons that have had their state changed.
    void update(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons);

    /// @brief Returns the object name.
    ///
    /// @return Object name.
    std::string getName();

    /// @brief Can be invoked by users of this object to get a string representation of the phones UI.
    ///
    /// @return String that represents a deskphones appearance.
    std::string getStateString();

    /// @brief Indicates whether the phone state is critical and should be forced onto the phone.
    ///
    /// @return \c true if the phone state is considered critical and the update should be pushed to the phone.
    bool isCritical();

    /// @brief Invoked by users of this object to populate a PJSIP notification action that the deskphone will
    ///        recognize.
    ///
    /// @param action PJSIP notification action to push the state to the phones.
    virtual void initialize(cpp_ami::action::PJSIPNotify &action) = 0;

    /// @brief Returns \c true if this phone UI is compatible with AoR \c aor.
    ///
    /// @param aor AoR to check.
    ///
    /// @return \c true if this phone UI is compatible with AoR \c aor.
    bool hasAoR(std::string const &aor);

    /// @brief Returns collection of compatible AoR's for this object.
    ///
    /// @return Collection of AoR's compatible with this object.
    std::vector<std::string> getAoRs();

    /// @brief Sets list of compatible AoR's for this object.
    ///
    /// @param aors Collection of compatible AoR's.
    void setAoRs(std::unordered_set<std::string> aors);

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
    /// @param critical Overrides the initial critical state of the UI. Setting this flag to \c true will force update
    ///                 the phone UI.
    ///
    /// @return XML state and critical flag.
    virtual std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::shared_ptr<button_state::PhoneButton> const &button, bool critical) = 0;

    /// @brief Creates the new phone UI state from the buttons states and initial critical state.
    ///
    /// @param buttons Buttons to create the new phone state XML from.
    /// @param critical Overrides the initial critical state of the UI. Setting this flag to \c true will force update
    ///                 the phone UL.
    ///
    /// @return XML state and critical flag.
    virtual std::pair<pugi::xml_document, bool>
        createPhoneStateXML(std::vector<std::shared_ptr<button_state::PhoneButton>> const &buttons, bool critical) = 0;

private:
    std::string name_;                                  ///< Name for the phone UL.
    std::shared_ptr<bridge::AoRProvider> provider_;          ///< Pointer to adapter that loads the AoRs.
    std::shared_ptr<PhoneUIState> cached_button_state_; ///< Current phone UI state.
    std::shared_mutex cached_button_state_mut_;         ///< Mutex on phone UI state.
    std::unordered_set<std::string> compatible_aors_;   ///< List of compatible AoRs.
    std::shared_mutex compatible_aors_mut_;             ///< Mutex on \c compatible_aors_.
};

} // namespace bridge

#endif
