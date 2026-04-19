// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/Adapter.hpp"

#include "phonebook/adapter/PjsipWizardAdapter.hpp"
#include <cassert>

using namespace phonebook;

std::shared_ptr<Adapter> Adapter::create(YAML::Node const &config, std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "wizard") {
        return adapter::PJSIPWizardAdapter::create(config, conn);
    }
    assert(false);
    return nullptr;
}