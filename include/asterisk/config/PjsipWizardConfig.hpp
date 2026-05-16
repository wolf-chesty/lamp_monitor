// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef ASTERISK_CONFIG_PJSIP_WIZARD_CONFIG_HPP
#define ASTERISK_CONFIG_PJSIP_WIZARD_CONFIG_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace asterisk::config {

class PJSIPWizardConfig {
public:
    using lambda_t = std::function<void(YAML::Node const &)>;

public:
    explicit PJSIPWizardConfig(std::shared_ptr<cpp_ami::Connection> io_conn);
    ~PJSIPWizardConfig() = default;

    void process(lambda_t const &lambda);

private:
    std::shared_ptr<cpp_ami::Connection> io_conn_;
};

} // namespace asterisk::config

#endif
