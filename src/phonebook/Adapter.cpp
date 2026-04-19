// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/Adapter.hpp"

#include "phonebook/adapter/PjsipWizardAdapter.hpp"
#include <cassert>

using namespace phonebook;

std::shared_ptr<Adapter> Adapter::create(std::shared_ptr<cpp_ami::Connection> const &connection,
                                         YAML::Node const &config)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "wizard") {
        return adapter::PJSIPWizardAdapter::create(connection, config);
    }
    assert(false);
    return nullptr;
}