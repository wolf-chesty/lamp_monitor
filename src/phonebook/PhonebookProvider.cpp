// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/PhonebookProvider.hpp"

#include "phonebook/provider/PjsipWizardProvider.hpp"
#include <cassert>

using namespace phonebook;

std::shared_ptr<PhonebookProvider> PhonebookProvider::create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "pjsip_wizard_cfg") {
        return phonebook::provider::PJSIPWizardProvider::create(config, conn);
    }
    assert(false);
    return nullptr;
}