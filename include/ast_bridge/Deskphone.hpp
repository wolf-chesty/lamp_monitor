// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_DESKPHONE_HPP
#define AST_BRIDGE_DESKPHONE_HPP

#include "cache/DeskphoneCache.hpp"
#include "button_state/LampField.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>

namespace ast_bridge {

/// @class Deskphone
/// @namespace ast_bridge
///
/// @brief Monitors the Asterisk server for deskphone registration events.
///
/// This object acts as an Asterisk to application state bridge. This object will monitor for Asterisk AMI PJSIP
/// registration events and will update the phone UI of newly registered deskphones. If the deskphone was recently
/// updated the phones TTL is updated and its UI updated depending on whether the current UI state is critical.
class Deskphone {
public:
    explicit Deskphone(std::shared_ptr<DeskphoneCache> deskphone_cache,
                       std::shared_ptr<button_state::LampField> lamp_field,
                       std::shared_ptr<cpp_ami::Connection> io_conn);
    ~Deskphone();

private:
    /// @brief Handles AMI events coming from the Asterisk AMI server.
    ///
    /// @param event Event sent by the Asterisk AMI server.
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    /// @brief Publish phone state to \c aor.
    ///
    /// @param aor AOR to send state to.
    /// @param phone_ui Phone UI renderer.
    void publishPhoneState(std::string const &aor, std::shared_ptr<ui::PhoneUI> const &phone_ui);

    std::shared_ptr<DeskphoneCache> deskphone_cache_;           ///< Deskphone cache.
    std::shared_ptr<button_state::LampField> lamp_field_;       ///< Lamp field state.
    std::shared_ptr<cpp_ami::Connection> io_conn_;              ///< Asterisk AMI connection.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< AMI callback handler ID.
};

} // namespace ast_bridge

#endif
