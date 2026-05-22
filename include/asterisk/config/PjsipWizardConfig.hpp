// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef ASTERISK_CONFIG_PJSIP_WIZARD_CONFIG_HPP
#define ASTERISK_CONFIG_PJSIP_WIZARD_CONFIG_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace asterisk::config {

/// @class PJSIPWizardConfig
/// @namespace asterisk::config
///
/// @brief Provides an interfaces to the Asterisk pjsip_wizard.conf configuration file.
class PJSIPWizardConfig {
public:
    using lambda_t = std::function<void(YAML::Node const &)>;

public:
    explicit PJSIPWizardConfig(std::shared_ptr<cpp_ami::Connection> io_conn);
    ~PJSIPWizardConfig() = default;

    /// @brief Invokes lambda on each element in the configuration file.
    ///
    /// @param lambda Function to invoke on each element in the configuration file.
    void process(lambda_t const &lambda);

private:
    std::shared_ptr<cpp_ami::Connection> io_conn_; ///< Asterisk AMI connection.
};

} // namespace asterisk::config

#endif
