// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_PHONEBOOK_HPP
#define XML_PHONEBOOK_HPP

#include <string>

namespace xml {

/// @class Phonebook
/// @namespace xml
///
/// @brief Provides an interface for classes that create XML phonebook data.
class Phonebook {
public:
    Phonebook() = default;
    virtual ~Phonebook() = default;

    /// @brief Returns an XML phonebook to the caller.
    ///
    /// @return String containing an XML phonebook.
    virtual std::string getPhonebookXML() = 0;
};

}

#endif
