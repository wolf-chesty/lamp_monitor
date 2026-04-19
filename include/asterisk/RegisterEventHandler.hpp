// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_REGISTER_EVENT_HANDLER_HPP
#define AST_BRIDGE_REGISTER_EVENT_HANDLER_HPP

#include "asterisk/EventHandler.hpp"

#include "button_state/ButtonPlan.hpp"
#include "cache/DeskphoneCache.hpp"
#include "ui/PhoneUi.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace asterisk {

/// @class RegisterEventHandler
/// @namespace asterisk
///
/// @brief Monitors the Asterisk server for deskphone registration events.
///
/// This object acts as an Asterisk to application state bridge. This object will monitor for Asterisk AMI PJSIP
/// registration events and will update the phone UI of newly registered deskphones. If the deskphone was recently
/// updated the phones TTL is updated and its UI updated depending on whether the current UI state is critical.
class RegisterEventHandler : public EventHandler {
public:
    explicit RegisterEventHandler(std::shared_ptr<DeskphoneCache> deskphone_cache,
                                  std::shared_ptr<cpp_ami::Connection> io_conn);
    ~RegisterEventHandler() override;

    /// @brief Returns type for this object.
    ///
    /// @return Event type.
    EventType getType() const override;

    /// @brief Adds a new button plan to this object.
    ///
    /// @param name Name of the button plan.
    /// @param button_plan Pointer to button plan.
    ///
    /// @return \c true if the button plan was successfully added to the object.
    bool addButtonPlan(std::string const &name, std::shared_ptr<button_state::ButtonPlan> const &button_plan);

private:
    /// @brief Returns a pointer to the button plan with  name \c name.
    ///
    /// @param name Name of button plan to retrieve.
    ///
    /// @return Pointer to save button plan.
    std::shared_ptr<button_state::ButtonPlan> getButtonPlan(std::string const &name);

    /// @brief Returns a phone UI renderer associated with \c plan_name and \c ui_name.
    ///
    /// @param plan_name Plan name containing the phone UI renderer.
    /// @param ui_name Phone UI renderer to retrieve from the button plan.
    ///
    /// @return Pointer to phone UI renderer.
    std::shared_ptr<ui::PhoneUI> getPhoneUI(std::string const &plan_name, std::string const &ui_name);

    /// @brief Handles AMI events coming from the Asterisk AMI server.
    ///
    /// @param event Event sent by the Asterisk AMI server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    /// @brief Publish phone state to \c aor.
    ///
    /// @param aor AOR to send state to.
    /// @param phone_ui Phone UI renderer.
    void publishPhoneState(std::string const &aor, std::shared_ptr<ui::PhoneUI> const &phone_ui);

    std::shared_ptr<DeskphoneCache> deskphone_cache_;                                         ///< Deskphone cache.
    std::unordered_map<std::string, std::shared_ptr<button_state::ButtonPlan>> button_plans_; ///< Button plans for app.
    std::shared_mutex button_plans_mut_;                        ///< Mutex on \c button_plans_.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< AMI callback handler ID.
};

} // namespace asterisk

#endif
