// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AST_BRIDGE_DESKPHONE_HPP
#define AST_BRIDGE_DESKPHONE_HPP

#include "button_state/LampField.hpp"
#include "DeskphoneCache.hpp"
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <memory>
#include <string>

namespace ast_bridge {

/// @class Deskphone
/// @namespace ast_bridge
///
/// @brief Monitors for deskphone registration events coming from the Asterisk server.
///
/// Objects of this class will monitor Asterisk AMI events for the occurrence of PJSIP registrations. Upon receiving a
/// new PJSIP registration the object will cache the SIP endpoint and notify the endpoint of the current button state.
///
/// Upon button state change this object will notify all deskphone endpoints in the cache of the new button state.
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

    void publishPhoneState(std::string const &aor, std::shared_ptr<ui::PhoneUI> const &phone_adapter);

    std::shared_ptr<DeskphoneCache> deskphone_cache_;           ///< Deskphone cache.
    std::shared_ptr<button_state::LampField> lamp_field_;       ///< Lamp field state.
    std::shared_ptr<cpp_ami::Connection> io_conn_;              ///< Asterisk AMI connection.
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< AMI callback handler ID.
};

} // namespace monitor

#endif
