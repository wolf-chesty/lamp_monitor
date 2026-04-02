// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_FIELD_STATE_HPP
#define LAMP_FIELD_STATE_HPP

#include <mutex>
#include <pugixml.hpp>
#include <string>

/// @class LampFieldState
class LampFieldState {
public:
    LampFieldState() = default;
    LampFieldState(pugi::xml_document xml, bool beep_required);
    LampFieldState(LampFieldState const &r);
    LampFieldState(LampFieldState &&r) noexcept;
    ~LampFieldState() = default;

    LampFieldState &operator=(LampFieldState const &r);
    LampFieldState &operator=(LampFieldState &&r) noexcept;

    pugi::xml_document const &getXML() const;

    std::string getXMLString();

    bool forceUpdate() const;

private:
    pugi::xml_document xml_;    ///< XML containing lamp field state.
    bool beep_required_{false}; ///< Flag indicating XML should be pushed to deskphones.
    std::mutex xml_str_mut_;    ///< Cached XML string representation of lamp field state XML.
    std::string xml_str_;       ///< Mutex controlling access to cached XML string.
};

#endif
