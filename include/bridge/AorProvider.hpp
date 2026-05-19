// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef AOR_PROVIDER_HPP
#define AOR_PROVIDER_HPP

#include <c++ami/Connection.hpp>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace bridge {

/// @class AoRProvider
/// @namespace bridge
///
/// @brief Objects of this type return AoR's that are compatible with a phone UI object.
class AoRProvider {
public:
    AoRProvider() = default;
    virtual ~AoRProvider() = default;

    /// @brief Factory function that will create the appropriate adapter object specified in \c config.
    ///
    /// @param config Configuration parameters.
    /// @param conn Pointer to AMI Asterisk connection.
    ///
    /// @return Pointer to new object.
    static std::shared_ptr<AoRProvider> create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &conn);

    /// @brief Returns a collection of compatible AoRs.
    ///
    /// @return Collection of compatible AoRs.
    virtual std::unordered_set<std::string> getCompatibleAoRs() = 0;
};

} // namespace bridge

#endif