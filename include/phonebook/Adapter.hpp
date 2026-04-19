// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_ADAPTER_HPP
#define PHONEBOOK_ADAPTER_HPP

#include "phonebook/CallerIdInfo.hpp"
#include <c++ami/Connection.hpp>
#include <memory>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace phonebook {

/// @class Adapter
/// @namespace phonebook
///
/// @brief Provides an adapting interface for objects that need application specific caller ID details.
///
/// Implementers of this interface will return a collection of caller ID details to be used by this application to
/// return to deskphones.
class Adapter {
public:
    Adapter() = default;
    virtual ~Adapter() = default;

    /// @brief Factory function that will create the appropriate adapter object specified in \c config.
    ///
    /// @param config Configuration parameters.
    /// @param conn Pointer to AMI Asterisk connection.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<Adapter> create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &conn);

    /// @brief Returns a collection of caller ID details.
    virtual std::vector<CallerIDInfo> getPhonebookDetails() const = 0;
};

} // namespace phonebook

#endif
