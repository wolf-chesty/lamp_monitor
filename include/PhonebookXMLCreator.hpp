// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_XML_CREATOR_HPP
#define PHONEBOOK_XML_CREATOR_HPP

#include <string>

class PhonebookXMLCreator {
public:
    virtual ~PhonebookXMLCreator() = default;

    virtual std::string getPhonebookXML() = 0;
};

#endif
