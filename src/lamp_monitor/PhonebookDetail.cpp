// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/PhonebookDetail.hpp"

bool PhonebookDetail::operator==(PhonebookDetail const &rhs) const
{
    return name == rhs.name && number == rhs.number;
}

auto PhonebookDetail::operator<=>(PhonebookDetail const &rhs) const
{
    if (auto const cmp = name <=> rhs.name; cmp != 0) {
        return cmp;
    }
    return number <=> rhs.number;
}
