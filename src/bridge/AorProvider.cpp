// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/AorProvider.hpp"

#include "bridge/provider/PjsipWizardProvider.hpp"
#include <cassert>

using namespace bridge;

std::shared_ptr<AoRProvider> AoRProvider::create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "pjsip_wizard_cfg") {
        return bridge::provider::PJSIPWizardProvider::create(config, conn);
    }
    return nullptr;
}