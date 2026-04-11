// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_PHONEBOOK_HPP
#define XML_YEALINK_PHONEBOOK_HPP

#include "xml/Phonebook.hpp"

#include "phonebook/Adapter.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

namespace xml::yealink {

/// @class Phonebook
/// @namespace xml::yealink
///
/// @brief Creates XML browser phonebook compatible with display on Yealink IP deskphones.
class Phonebook : public xml::Phonebook {
public:
    using clock_t = std::chrono::steady_clock;

public:
    explicit Phonebook(std::shared_ptr<phonebook::Adapter> phonebook_provider, std::chrono::minutes expiry);
    ~Phonebook() override = default;

    /// @brief Returns XML browser phonebook.
    ///
    /// @return String containing XML browser phonebook compatible with Yealink IP deskphones.
    std::string getPhonebookXML() override;

private:
    std::shared_ptr<phonebook::Adapter> phonebook_adapter_; ///< Pointer to source phonebook adapter.
    clock_t::time_point timestamp_{};                       ///< Timestamp for last time XML data was created.
    std::chrono::minutes expiry_;                           ///< How long XML phonebooks should be considered vaild.
    std::string phonebook_xml_;                             ///< Cached XML browser phonebook data.
    std::mutex phonebook_xml_mut_;                          ///< Mutex to control phonebook creation.
};

} // namespace xml::yealink

#endif
