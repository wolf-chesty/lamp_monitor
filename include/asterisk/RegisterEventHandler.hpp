// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef ASTERISK_REGISTER_EVENT_HANDLER_HPP
#define ASTERISK_REGISTER_EVENT_HANDLER_HPP

#include "asterisk/EventHandler.hpp"

#include "bridge/PhoneStateDispatcher.hpp"
#include "bridge/PhoneUi.hpp"
#include "button_state/ButtonPlan.hpp"
#include "cache/DeskphoneCache.hpp"
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>
#include <vector>

namespace asterisk {

/// @class RegisterEventHandler
/// @namespace asterisk
///
/// @brief Monitors the Asterisk server for deskphone registration events.
///
/// This object acts as an Asterisk to application state bridge. This object will monitor for Asterisk AMI PJSIP
/// registration events and update the UI of newly registered deskphones or if the UI needs to be updated.
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
    /// @param button_plan Pointer to button plan.
    void registerButtonPlan(std::shared_ptr<button_state::ButtonPlan> const &button_plan);

private:
    /// @brief Handles AMI events coming from the Asterisk AMI server.
    ///
    /// @param event Event sent by the Asterisk AMI server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    /// @brief Publish phone state to \c aor.
    ///
    /// @param new_aor Flag indicating the AoR is a new registration.
    /// @param aor AoR to send state to.
    void publishPhoneState(bool const new_aor, std::string const &aor);

    std::shared_ptr<DeskphoneCache> deskphone_cache_;                     ///< Deskphone cache.
    std::vector<std::shared_ptr<button_state::ButtonPlan>> button_plans_; ///< Button plans for app.
    std::mutex button_plans_mut_;                                         ///< Mutex on \c button_plans_.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_;           ///< AMI callback handler ID.
};

} // namespace asterisk

#endif
