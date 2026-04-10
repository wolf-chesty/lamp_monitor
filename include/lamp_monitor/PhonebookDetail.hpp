// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_DETAIL_HPP
#define PHONEBOOK_DETAIL_HPP

#include <string>

struct PhonebookDetail {
    std::string name;
    std::string number;

    bool operator==(PhonebookDetail const &rhs) const;
    auto operator<=>(PhonebookDetail const &rhs) const;
};

#endif
