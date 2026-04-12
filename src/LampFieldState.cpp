// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "LampFieldState.hpp"

#include <sstream>

LampFieldState::LampFieldState(pugi::xml_document xml, bool is_critical)
    : xml_(std::move(xml))
    , is_critical_(is_critical)
{
}

pugi::xml_document const &LampFieldState::getXML() const
{
    return xml_;
}

std::string LampFieldState::toString()
{
    std::lock_guard const lock(xml_str_mut_);
    if (xml_str_.empty()) {
        xml_str_ = toString(xml_);
    }
    return xml_str_;
}

std::string LampFieldState::toString(pugi::xml_document const &xml)
{
    std::ostringstream xml_stream;
    xml.save(xml_stream, "", pugi::format_raw);
    return xml_stream.str();
}

bool LampFieldState::isCritical() const
{
    return is_critical_;
}
