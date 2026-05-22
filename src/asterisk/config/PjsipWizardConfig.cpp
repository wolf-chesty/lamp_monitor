// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "asterisk/config/PjsipWizardConfig.hpp"

#include <c++ami/action/GetConfigJson.hpp>
#include <cassert>
#include <syslog.h>

using namespace asterisk::config;

PJSIPWizardConfig::PJSIPWizardConfig(std::shared_ptr<cpp_ami::Connection> io_conn)
    : io_conn_(std::move(io_conn))
{
    assert(io_conn_);
}

void PJSIPWizardConfig::process(lambda_t const &lambda)
{
    cpp_ami::action::GetConfigJSON action;
    action["Filename"] = "pjsip_wizard.conf";
    auto const reaction = io_conn_->invoke(action);
    if (!reaction->isSuccess()) {
        return;
    }

    reaction->forEach([&lambda](cpp_ami::event::Event const &event) mutable -> bool {
        // Iterate over AoR records, invoking lambda on each record
        auto const &json = event["JSON"];
        for (auto const &aor_rec : YAML::Load(json)) {
            lambda(aor_rec.second);
        }
        return true;
    });
}
