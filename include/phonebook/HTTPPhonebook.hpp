// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_PHONEBOOK_STRING_CREATOR_HPP
#define PHONEBOOK_PHONEBOOK_STRING_CREATOR_HPP

#include "phonebook/Adapter.hpp"
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

namespace phonebook {

/// @class HTTPPhonebook
/// @namespace phonebook
///
/// @brief Provides an interface for classes that create phonebook text that is compatible with deskphones.
class HTTPPhonebook {
public:
    HTTPPhonebook(std::shared_ptr<Adapter> phonebook_adapter);
    virtual ~HTTPPhonebook() = default;

    /// @brief Creates a new object using parameters specified in \c config.
    ///
    /// @param config Configuration parameters for this object.
    /// @param adapter Pointer to datasource adapter.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<HTTPPhonebook> create(YAML::Node const &config, std::shared_ptr<Adapter> const &adapter);

    /// @brief Returns a phonebook string compatible with a deskphone.
    ///
    /// @return String containing phonebook string compatible with a deskphone.
    ///
    /// Implementors of this function will need to create a string containing phonebook data that is
    /// compatible with their deskphone.
    virtual std::string getPhonebook() = 0;

    /// @brief Returns the HTTP content type for this object.
    ///
    /// @return HTTP content type for this object.
    virtual std::string getContentType() = 0;

protected:
    /// @brief Returns the phonebook adapter for this object.
    ///
    /// @return Pointer to the phonebook adapter for this object.
    std::shared_ptr<Adapter> getPhonebookAdapter();

private:
    std::shared_ptr<Adapter> phonebook_adapter_; ///< Pointer to source phonebook adapter.
};

} // namespace phonebook

#endif
