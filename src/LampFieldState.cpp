// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "LampFieldState.hpp"

#include <sstream>

LampFieldState::LampFieldState(pugi::xml_document xml, bool beep_required)
    : xml_(std::move(xml))
    , beep_required_(beep_required)
{
}

LampFieldState::LampFieldState(LampFieldState const &r)
    : beep_required_(r.beep_required_)
{
    xml_.reset(r.xml_);
}

LampFieldState::LampFieldState(LampFieldState &&r) noexcept
    : xml_(std::move(r.xml_))
    , beep_required_(r.beep_required_)
{
}

LampFieldState &LampFieldState::operator=(LampFieldState const &r)
{
    xml_.reset(r.xml_);
    beep_required_ = r.beep_required_;
    return *this;
}

LampFieldState &LampFieldState::operator=(LampFieldState &&r) noexcept
{
    xml_ = std::move(r.xml_);
    beep_required_ = r.beep_required_;
    return *this;
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
    return beep_required_;
}
