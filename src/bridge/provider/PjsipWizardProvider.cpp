// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/provider/PjsipWizardProvider.hpp"

#include <syslog.h>

using namespace bridge::provider;

PJSIPWizardProvider::PJSIPWizardProvider(std::shared_ptr<cpp_ami::Connection> io_conn, std::string match,
                                       match::SymbolMap symbol_map)
    : config_(std::move(io_conn))
    , matcher_(std::move(match), std::move(symbol_map))
{
}

std::shared_ptr<PJSIPWizardProvider> PJSIPWizardProvider::create(YAML::Node const &config,
                                                               std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const match = config["match"].as<std::string>();
    auto const symbol_map = match::SymbolMap::create(config["symbol_maps"]);
    return std::make_shared<PJSIPWizardProvider>(conn, match, symbol_map);
}

/// Parses the Asterisk pjsip_wizard.conf file, returning the AoR for each AOR endpoint found.
std::unordered_set<std::string> PJSIPWizardProvider::getCompatibleAoRs()
{
    std::unordered_set<std::string> compatible_aors;
    config_.process([this, &compatible_aors](YAML::Node const &aor_cfg_json) mutable -> void {
        if (!matcher_.isMatch(aor_cfg_json)) {
            return;
        }
        auto const &endpoint_auth_json =aor_cfg_json["endpoint/auth"];
        if (!endpoint_auth_json) {
            syslog(LOG_WARNING, "Missing endpoint/auth");
            return;
        }
        auto const aor = endpoint_auth_json.as<std::string>();
        compatible_aors.emplace(aor);
    });
    return compatible_aors;
}
