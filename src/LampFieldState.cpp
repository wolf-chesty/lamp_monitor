// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "LampFieldState.hpp"

#include <sstream>

LampFieldState::LampFieldState(pugi::xml_document xml, bool force_update)
    : xml_(std::move(xml))
    , force_update_(force_update)
{
}

pugi::xml_document const &LampFieldState::getXML() const
{
    return xml_;
}

std::string LampFieldState::getXMLString()
{
    std::lock_guard const lock(xml_str_mut_);
    if (xml_str_.empty()) {
        std::ostringstream xml_stream;
        xml_.save(xml_stream, "", pugi::format_raw);
        xml_str_ = xml_stream.str();
    }
    return xml_str_;
}

bool LampFieldState::forceUpdate() const
{
    return force_update_;
}
