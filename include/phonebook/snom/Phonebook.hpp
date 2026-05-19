// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_SNOM_PHONEBOOK_HPP
#define PHONEBOOK_SNOM_PHONEBOOK_HPP

#include "phonebook/Phonebook.hpp"

namespace phonebook::snom {

/// @class Phonebook
/// @namespace phonebook::snom
///
/// @brief Creates XML browser phonebook compatible with display on Snom IP deskphones.
class Phonebook : public phonebook::Phonebook {
public:
    explicit Phonebook(std::shared_ptr<phonebook::PhonebookProvider> phonebook_adapter, std::chrono::minutes expiry);
    ~Phonebook() override = default;

    /// @brief Creates a new object using parameters from \c config.
    ///
    /// @param config Configuration options.
    /// @param adapter Pointer to datasource adapter.
    static std::shared_ptr<phonebook::Phonebook> create(YAML::Node const &config,
                                                            std::shared_ptr<phonebook::PhonebookProvider> const &adapter);

    /// @brief Returns the HTTP content type for text created by this object.
    ///
    /// @return HTTP content type for text created by this object.
    std::string getContentType() override;

protected:
    /// @brief Returns XML browser phonebook.
    ///
    /// @param phonebook_source Phonebook data provider.
    ///
    /// @return String containing XML browser phonebook compatible with Snom IP deskphones.
    std::string getPhonebook(std::shared_ptr<PhonebookProvider> const &phonebook_source) override;
};

}

#endif
