// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BUTTON_STATE_LAMP_FIELD_HPP
#define BUTTON_STATE_LAMP_FIELD_HPP

#include "asterisk/EventHandler.hpp"
#include "button_state/PhoneButton.hpp"
#include "ui/PhoneEventDispatcher.hpp"
#include "ui/PhoneUi.hpp"
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace button_state {

/// @class ButtonPlan
/// @namespace button_state
///
/// @brief This object represents the current button group state of a deskphone.
///
/// This object is the applications representation of a deskphones button group. As such, each deskphone type will need
/// to have a button plan object created for that particular device. If deskphones share a button layout, then perhaps
/// button plan objects can be shared between deskphones.
///
/// This object also has a list of phone UI objects that are responsible for rendering button state of the physical
/// hardware deskphones. As button states occur this object will use this collection of phone UI objects to render the
/// current deskphone button state and send that information to each phone on the network.
class ButtonPlan {
public:
    explicit ButtonPlan(std::string name, std::shared_ptr<ui::PhoneEventDispatcher> ami_bridge);
    ~ButtonPlan() = default;

    /// @brief Creates a new object using configuration paramters from \c config.
    ///
    /// @param config Configuration parameters for object.
    /// @param ami_bridge Pointer to object that pushes phone state to deskphones.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<ButtonPlan> create(YAML::Node const &config,
                                              std::shared_ptr<ui::PhoneEventDispatcher> const &ami_bridge);

    /// @brief Returns the name of the object.
    ///
    /// @return Name of object.
    std::string const &getName() const;

    /// @brief Adds button state object.
    ///
    /// @param button_id ID of button.
    /// @param button Pointer to button state.
    ///
    /// @return \c true if button state with \c button_id was successfully added.
    bool addButton(uint16_t const button_id, std::shared_ptr<PhoneButton> const &button);

    /// @brief Returns button for \c button_id.
    ///
    /// @param button_id Button ID of button to get from collection of monitored buttons.
    std::shared_ptr<PhoneButton> getButton(uint16_t const button_id);

    /// @brief Removes button with button id of \c button_id from collection of buttons being monitored.
    ///
    /// @param button_id Button ID to remove from collection of monitored buttons.
    void removeButton(uint16_t const button_id);

    /// @brief Returns collection of buttons.
    ///
    /// @return Collection of button objects monitored by this object.
    std::vector<std::shared_ptr<PhoneButton>> getButtons();

    /// @brief Add phone UI to this lamp field.
    ///
    /// @param ui_name Phone UI name.
    /// @param ui Phone UI object.
    bool registerUI(std::string const &ui_name, std::shared_ptr<ui::PhoneUI> const &ui);

    /// @brief Remove phone UI from this lamp field.
    ///
    /// @param ui_name Phone UI name.
    void unregisterUI(std::string const &ui_name);

    /// @brief Gets the phone UI for \c ui_name.
    ///
    /// @param ui_name Phone UI key.
    ///
    /// @return Pointer to phone UI object.
    std::shared_ptr<ui::PhoneUI> getPhoneUI(std::string const &ui_name);

    /// @brief Invalidates the lamp field object.
    ///
    /// @param button_id ID of button that invalidated the object.
    ///
    /// This function is invoked whenever a button state changes.
    void invalidate(uint16_t const button_id);

    /// @brief Adds an event handler to this object.
    ///
    /// @param id ID of the event handler.
    /// @param event_handler Pointer to event handler.
    ///
    /// @return \c true if event handler with \c id was successfully added.
    bool addEventHandler(uint16_t id, std::shared_ptr<asterisk::EventHandler> const &event_handler);

    /// @brief Returns event handler with ID \c id.
    ///
    /// @param id ID of event handler to retrieve.
    ///
    /// @return Pointer to event handler.
    std::shared_ptr<asterisk::EventHandler> getEventHandler(uint16_t const id);

private:
    std::string name_;                                                        ///< Plan name.
    std::shared_ptr<ui::PhoneEventDispatcher> ami_bridge_;                    ///< Bridge to deskphones.
    std::unordered_map<uint16_t, std::shared_ptr<PhoneButton>> buttons_;      ///< Collection of observed buttons.
    std::shared_mutex buttons_mut_;                                           ///< Mutex on button collection.
    std::unordered_map<std::string, std::shared_ptr<ui::PhoneUI>> phone_uis_; ///< Phone UI's to render this object.
    std::shared_mutex phone_uis_mut_;                                         ///< Mutex on phone UI collection.
    std::unordered_map<uint16_t, std::shared_ptr<asterisk::EventHandler>>
        event_handlers_; ///< Collection of event handlers that can change button states managed by this object.
    std::shared_mutex event_handlers_mut_; ///< Mutex on \c event_handlers_.
};

} // namespace button_state

#endif
