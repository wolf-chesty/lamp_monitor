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

YAML::Node loadJSON(std::string const &json_str)
{
    try {
        return YAML::Load(json_str);
    }
    catch (std::exception const &e) {
        syslog(LOG_ERR, "Unable to parse pjsip_wizard.conf");
    }
    return YAML::Node();
}

void PJSIPWizardConfig::process(lambda_t const &lambda)
{
    cpp_ami::action::GetConfigJSON action;
    action["Filename"] = "pjsip_wizard.conf";
    if (auto const reaction = io_conn_->invoke(action); reaction->isSuccess()) {
        reaction->forEach([&lambda](cpp_ami::event::Event const &event) mutable -> bool {
            // Iterate over AoR records, collecting all variables for use
            for (auto const &aor_rec : loadJSON(event["JSON"])) {
                lambda(aor_rec.second);
            }
            return true;
        });
    }
}
