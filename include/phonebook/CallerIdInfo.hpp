// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_CALLER_ID_INFO_HPP
#define PHONEBOOK_CALLER_ID_INFO_HPP

#include <string>

namespace phonebook {

/// @struct CallerIDInfo
/// @namespace phonebook
///
/// @brief Contains data for a caller ID record.
struct CallerIDInfo {
    std::string name;   ///< Caller ID name.
    std::string number; ///< Caller ID number.
};

} // namespace phonebook

#endif
