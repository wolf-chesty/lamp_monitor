// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_PROVIDER_HPP
#define PHONEBOOK_PROVIDER_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace phonebook {

/// @class PhonebookProvider
/// @namespace phonebook
///
/// @brief Provides an interface for objects that need application specific caller ID details.
///
/// Implementers of this interface will return a collection of caller ID details to be used by this application to
/// create phonebooks.
class PhonebookProvider {
public:
    /// @struct CallerIDDetails
    /// @namespace phonebook
    ///
    /// @brief Contains data for a caller ID record.
    struct CallerIDDetails {
        std::string name;   ///< Caller ID name.
        std::string number; ///< Caller ID number.
    };

public:
    PhonebookProvider() = default;
    virtual ~PhonebookProvider() = default;

    /// @brief Factory function that will create the appropriate adapter object specified in \c config.
    ///
    /// @param config Configuration parameters.
    /// @param conn Pointer to AMI Asterisk connection.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<PhonebookProvider> create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &conn);

    /// @brief Returns a collection of caller ID details.
    ///
    /// @return Collection of caller ID details.
    virtual std::vector<CallerIDDetails> getCallerDetails() = 0;
};

} // namespace phonebook

#endif
