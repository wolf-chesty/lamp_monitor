// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_YEALINK_PHONEBOOK_HPP
#define PHONEBOOK_YEALINK_PHONEBOOK_HPP

#include "phonebook/Phonebook.hpp"

namespace phonebook::yealink {

/// @class Phonebook
/// @namespace phonebook::yealink
///
/// @brief Creates XML browser phonebook compatible with display on Yealink IP deskphones.
class Phonebook : public phonebook::Phonebook {
public:
    explicit Phonebook(std::shared_ptr<phonebook::PhonebookProvider> phonebook_provider, std::chrono::minutes expiry);
    ~Phonebook() override = default;

    /// @brief Returns the HTTP content type for text created by this object.
    ///
    /// @return HTTP content type for text created by this object.
    std::string getContentType() override;

protected:
    /// @brief Returns XML browser phonebook.
    ///
    /// @param phonebook_source Phonebook data provider.
    ///
    /// @return String containing XML browser phonebook compatible with Yealink IP deskphones.
    std::string getPhonebook(std::shared_ptr<PhonebookProvider> const &phonebook_source) override;
};

} // namespace xml::yealink

#endif
