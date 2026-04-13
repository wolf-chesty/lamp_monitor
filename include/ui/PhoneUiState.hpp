// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef UI_PHONE_UI_STATE_HPP
#define UI_PHONE_UI_STATE_HPP

#include <mutex>
#include <pugixml.hpp>
#include <string>

namespace ui {

/// @class PhoneUIState
/// @namespace monitor
///
/// @brief Object contains the current state for deskphone UI.
class PhoneUIState {
public:
    explicit PhoneUIState(pugi::xml_document xml, bool is_critical);
    ~PhoneUIState() = default;

    /// @brief Return XML object containing current phone button state.
    ///
    /// @return XML object.
    pugi::xml_document const &getXML() const;

    /// @brief Returns if the XML state is critical in nature.
    ///
    /// @return \c true if the XML state should be pushed to phone even during idle times.
    bool isCritical() const;

    /// @brief Returns a string representation of the XML phone state.
    ///
    /// @return String containing XML body.
    std::string toString();

    /// @brief Converts \c xml to a string.
    ///
    /// @param xml XML object containing screen state data.
    ///
    /// @return String representation of XML data.
    static std::string toString(pugi::xml_document const &xml);

private:
    pugi::xml_document xml_;  ///< XML containing lamp field state.
    bool is_critical_{false}; ///< Flag indicating XML should be pushed to deskphones.
    std::string xml_str_;     ///< Mutex controlling access to cached XML string.
    std::mutex xml_str_mut_;  ///< Cached XML string representation of lamp field state XML.
};

} // namespace monitor

#endif
