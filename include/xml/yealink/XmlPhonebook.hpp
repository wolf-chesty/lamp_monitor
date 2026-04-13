// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_XML_PHONEBOOK_HPP
#define XML_YEALINK_XML_PHONEBOOK_HPP

#include "phonebook/PhonebookStringCreator.hpp"

#include <chrono>
#include <mutex>
#include <string>

namespace xml::yealink {

/// @class XMLPhonebook
/// @namespace xml::yealink
///
/// @brief Creates XML browser phonebook compatible with display on Yealink IP deskphones.
class XMLPhonebook : public phonebook::PhonebookStringCreator {
public:
    using clock_t = std::chrono::steady_clock;

public:
    explicit XMLPhonebook(std::shared_ptr<phonebook::Adapter> phonebook_adapter, std::chrono::minutes expiry);
    ~XMLPhonebook() override = default;

    /// @brief Returns XML browser phonebook.
    ///
    /// @return String containing XML browser phonebook compatible with Yealink IP deskphones.
    std::string getPhonebookString() override;

private:
    clock_t::time_point timestamp_{}; ///< Timestamp for last time XML data was created.
    std::chrono::minutes expiry_;     ///< How long XML phonebooks should be considered vaild.
    std::string phonebook_xml_;       ///< Cached XML browser phonebook data.
    std::mutex phonebook_xml_mut_;    ///< Mutex to control phonebook creation.
};

} // namespace xml::yealink

#endif
