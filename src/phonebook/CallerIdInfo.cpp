// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/CallerIdInfo.hpp"

using namespace phonebook;

bool CallerIDInfo::operator==(CallerIDInfo const &rhs) const
{
    return name == rhs.name && number == rhs.number;
}

auto CallerIDInfo::operator<=>(CallerIDInfo const &rhs) const
{
    if (auto const cmp = name <=> rhs.name; cmp != 0) {
        return cmp;
    }
    return number <=> rhs.number;
}
