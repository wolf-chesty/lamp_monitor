// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_FIELD_STATE_HPP
#define LAMP_FIELD_STATE_HPP

#include <mutex>
#include <pugixml.hpp>
#include <string>

/// @class LampFieldState
///
/// @brief Object contains the current lamp field state for deskphones.
class LampFieldState {
public:
    explicit LampFieldState(pugi::xml_document xml, bool is_critical);
    ~LampFieldState() = default;

    pugi::xml_document const &getXML() const;

    std::string toString();
    static std::string toString(pugi::xml_document const &xml);

    bool isCritical() const;

private:
    pugi::xml_document xml_;  ///< XML containing lamp field state.
    bool is_critical_{false}; ///< Flag indicating XML should be pushed to deskphones.
    std::mutex xml_str_mut_;  ///< Cached XML string representation of lamp field state XML.
    std::string xml_str_;     ///< Mutex controlling access to cached XML string.
};

#endif
