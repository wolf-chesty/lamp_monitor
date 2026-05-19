// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_PHONEBOOK_HPP
#define PHONEBOOK_PHONEBOOK_HPP

#include "phonebook/PhonebookProvider.hpp"
#include <c++ami/Connection.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <yaml-cpp/yaml.h>

namespace phonebook {

/// @class Phonebook
/// @namespace phonebook
///
/// @brief Provides an interface for classes that create phonebook text that is compatible with deskphones.
class Phonebook {
public:
    using clock_t = std::chrono::steady_clock;

public:
    explicit Phonebook(std::shared_ptr<PhonebookProvider> phonebook_adapter, std::chrono::minutes expiry);
    virtual ~Phonebook() = default;

    /// @brief Creates a new object using parameters specified in \c config.
    ///
    /// @param config Configuration parameters for this object.
    /// @param source Pointer to datasource adapter.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<Phonebook> create(YAML::Node const &config,
                                             std::shared_ptr<PhonebookProvider> const &source);

    /// @brief Returns a phonebook string compatible with a deskphone.
    ///
    /// @return String containing phonebook string compatible with a deskphone.
    ///
    /// Implementors of this function will need to create a string containing phonebook data that is
    /// compatible with their deskphone.
    std::string getPhonebook();

    /// @brief Returns the HTTP content type for this object.
    ///
    /// @return HTTP content type for this object.
    virtual std::string getContentType() = 0;

protected:
    virtual std::string getPhonebook(std::shared_ptr<PhonebookProvider> const &phonebook_source) = 0;

private:
    std::shared_ptr<PhonebookProvider> phonebook_source_; ///< Pointer to source phonebook adapter.
    clock_t::time_point timestamp_{};                     ///< Timestamp for last time XML data was created.
    std::chrono::minutes expiry_;                         ///< How long XML phonebooks should be considered valid.
    std::string cached_phonebook_;                        ///< Cached XML browser phonebook data.
    std::mutex cached_phonebook_mut_;                     ///< Mutex to control phonebook creation.
};

} // namespace phonebook

#endif
