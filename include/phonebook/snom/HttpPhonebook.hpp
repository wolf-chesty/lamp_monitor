// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_SNOM_HTTP_PHONEBOOK_HPP
#define PHONEBOOK_SNOM_HTTP_PHONEBOOK_HPP

#include "phonebook/HttpPhonebook.hpp"

namespace phonebook::snom {


/// @class HTTPPhonebook
/// @namespace phonebook::snom
///
/// @brief Creates XML browser phonebook compatible with display on Snom IP deskphones.
class HTTPPhonebook : public phonebook::HTTPPhonebook {
public:
    explicit HTTPPhonebook(std::shared_ptr<phonebook::Adapter> phonebook_adapter, std::chrono::minutes expiry);
    ~HTTPPhonebook() override = default;

    /// @brief Creates a new object using parameters from \c config.
    ///
    /// @param config Configuration options.
    /// @param adapter Pointer to datasource adapter.
    static std::shared_ptr<phonebook::HTTPPhonebook> create(YAML::Node const &config,
                                                            std::shared_ptr<phonebook::Adapter> const &adapter);

    /// @brief Returns the HTTP content type for text created by this object.
    ///
    /// @return HTTP content type for text created by this object.
    std::string getContentType() override;

protected:
    /// @brief Returns XML browser phonebook.
    ///
    /// @return String containing XML browser phonebook compatible with Snom IP deskphones.
    std::string getPhonebookImpl() override;
};

}

#endif
